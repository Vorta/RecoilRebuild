#pragma once

#include "recoil/recoil_types.h"
#include <cstdarg>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <new>
#include <vector>

#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_dialog_host.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zInput/zinput.h"
#include "recoil/recoil_callconv.h"
#include "zclip_alt.h"

#include <windows.h>

struct zVec3;
struct zTag4Partial;
struct HudUiContainer;
struct HudUiBackground;
struct HudUiTextInput;
struct HudUiNetGameSetupPanel;

struct HudUiElement;
struct HudUiPanel;
struct HudUiCounter;
struct HudUiMessage;
struct HudUiStringMenu;
struct HudUiShieldMessageWidget;
struct HudUiStatsListElement;
struct HudUiTextStack4;
struct HudUiTimerPanel;
struct HudUiTimerPanelFloat;
struct HudUiCounterTextPanel;
struct HudUiTripletPanel;
struct HudUiNanitePanel;
struct HudUiTransitionTextPanel;
struct HudUiScoreboardEntry;
struct HudUiTriplet;
struct HudUiCircle;
struct HudUiBar;
struct HudUiZrdWidgetEx17C;
struct HudUiSlot;
struct HudUiManagerMeterBaseCandidate;
struct HudUiManagerMeterCandidate;
struct HudUiShieldMeterCandidate;
struct GameNetPlayerRow;
struct zZbdSectionCallbackCtx;
struct zSndSample;
struct zSndPlayHandle;
struct zFMV_Stream;

struct zTimedTask {
    zTimedTask *next;
    int kind;
    int flags;
    float remainingSeconds;
    int actionArg0;
    int actionArg1;
    int actionArg2;
    int actionArg3;
    int actionArg4;
    unsigned char payload_24[0x94];
    int alphaPointCount;
    int alphaVariantIndex;
    int alpha255;
    unsigned char payload_c4[0x48];
    int rasterVertexCount;
    int rasterDrawParam;

    void RemoveFromActiveList();
    void RunImmediateAction();
    static void TickActiveList();
};

RECOIL_STATIC_ASSERT(offsetof(zTimedTask, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, kind) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, flags) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, remainingSeconds) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg0) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, actionArg4) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaPointCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alphaVariantIndex) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, alpha255) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterVertexCount) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(zTimedTask, rasterDrawParam) == 0x110);

extern int g_zTimedTask_ActiveCount;
extern zTimedTask *g_zTimedTask_ActiveHead;
extern zTimedTask *g_zTimedTask_ActiveTail;
extern "C" unsigned int g_HudUi_InvalidateMask;
extern char g_HudFontName_Arial[];

extern int g_HudCmdMouseDebounceFrames;
struct HudUiWidget;
struct HudUiMgrData;
union HudUiMgrDataStorage;
struct PlayerProgressTargetSlotRuntime;
struct HudUiNetGameSetupOverlayOwner;
union HudUiNetGameSetupOverlayOwnerStorage;

struct HudUiMgrSensorTrackNode {
    int trackKind;
    void *payload;
    HudUiMgrSensorTrackNode *next;
};

enum HudUiMgrSensorTrackKind { HUD_SENSOR_TRACK_KIND_PLAYER = 2, HUD_SENSOR_TRACK_KIND_TURRET = 3 };

struct HudUiMgrSensorTrackList {
    int trackListAux;
    HudUiMgrSensorTrackNode *head;
    HudUiMgrSensorTrackNode *tail;
    int count;
};

extern "C" {
extern HudUiMgrSensorTrackList g_HudUiMgrSensor_TrackList;
}

extern zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage;
extern HudUiMgrDataStorage g_HudUiMgr;
extern HudUiTimerPanel *g_HudUiMgrTimerPanel;
extern HudUiTimerPanelFloat *g_HudUiMgrTimerPanelFloat;
extern HudUiStringMenu *g_HudUiMgrStringMenu;
extern HudUiStatsListElement *g_HudUiMgrStatsList;
extern zVidImagePartial *g_HudUiMgrSensorTargetMarkerImages[5];
extern HudUiMessage g_HudUiMgrMessages[10];
extern int g_HudUiMgrActiveWeaponMessageIndex;
extern int g_HudUiMgrActiveWeaponSideIndex;
extern HudUiCounter g_HudUiMgrModeCounters[4];
extern HudUiSlot g_HudUiMgrWeaponSlots[32];
extern HudUiSlot *g_HudUiMgrSensorTrackedProgressSlot;
extern int g_HudUiMgrSensor_RoundRobinTrackIndex;
extern int g_HudUiMgrActiveModeCounterIndex;
extern int g_HudUiMgrSensorTargetMarkerCount;
extern int g_HudUiMgrWeaponState;
extern HudUiNetGameSetupOverlayOwnerStorage g_HudUiNetGameSetupOverlayOwner;

struct HudUiRect {
    int left;
    int top;
    int right;
    int bottom;
};

struct HudUiRectDirty {
    unsigned int framesRemaining;
    int drawX;
    int drawY;
    int srcLeft;
    int srcTop;
    int srcRight;
    int srcBottom;
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-element.type
 * @recoil-artifact emits .text recoil:function:0x404d70: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudUiElement type.
 */
struct HudUiElement {
    HudUiElement *next;
    void *parent;
    unsigned int flags;
    float timer;
    int x;
    int y;
    void *bltSource;
    HudUiRect clipRect;
    unsigned short state;
    unsigned short padding32;

    /**
     * Source model note: Source-faithful helper recovered from address-backed callers in this
     * source file.
 * Purpose: run the recovered HudUiElement::~HudUiElement teardown path.
 */
HudUiElement() {
    }
    HudUiElement(
        int x,
        int y
    );
    HudUiElement(const HudUiElement &source);
    /**
     * Purpose: reset the HudUiElement virtual table during class destruction.
     */
    virtual ~HudUiElement() {}
    HudUiElement * Constructor(
        int x,
        int y
    );
    HudUiElement & operator=(const HudUiElement &source);
    virtual void Draw();
    virtual void DrawBase();
    virtual void SetPos(
        int x,
        int y
    );
    virtual void SetX(int x);
    virtual void SetY(int y);
    virtual void SetBltSourceAndClipRect(
        void *bltSourceOrNull,
        const HudUiRect *rectOrNull
    );
    virtual void SetClipRect(const HudUiRect *clipRect);
    virtual void Invalidate();
    virtual void Update(float deltaSeconds);
    virtual void OnUpdateIdle(float deltaSeconds);
    virtual HudUiRect * GetBoundsRectOrNull();
    virtual void OnActivate();
    virtual void OnClearBinding();
    virtual void OnHoverRepeat();
    virtual void ShowPreview();
    virtual void HidePreview();
    virtual void OnBeginCapture();
    virtual void OnEndCapture();
    virtual void OnPointerButtonState(
        int x,
        int y
    );
    virtual void OnCapturedPrimaryRelease();
    virtual int ShouldHandleInput(
        HudUiBackground *background,
        int hovered
    );
    virtual void AfterInputUpdate(
        HudUiBackground *background,
        int hovered
    );
    virtual int HitTest(
        int px,
        int py
    );
    virtual void SetVisible(int visible);
    virtual int GetCenterX();
    virtual int GetCenterY();
    virtual void EnableWordWrapWithRect(const HudUiRect *rect);
    virtual void GetTextRect(HudUiRect *outRect);
    void SetTimer(float duration);
    unsigned char HitTestTrue(
        int px,
        int py
    );
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-widget.type
 * @recoil-artifact emits .text recoil:function:0x4b3ce0: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * @recoil-artifact emits .text recoil:function:0x40d5f0: VC5 destructor cleanup forwarding thunk to the HudUiWidget destructor core.
 * Purpose: Record compiler-generated lifecycle and cleanup code emitted by the complete HudUiWidget type.
 */
struct HudUiWidget : HudUiElement {
    unsigned int ownsImage;
    unsigned int dirtyRectCount;
    zVidImagePartial *image;
    unsigned int imageStateWord;
    HudUiRect *bltClipRectOrNull;
    unsigned int alignFlags;
    HudUiRectDirty dirtyRects[4];

    HudUiWidget();
    HudUiWidget(unsigned int alignFlags);
    ~HudUiWidget();
    HudUiWidget * Constructor(unsigned int alignFlags);
    void DestructorCore();
    void ReleaseImageIfOwned();
    zVidImagePartial * SetImageByPathOwned(const char *imagePath);
    zVidImagePartial * SetImageBorrowedAndInvalidate(
        zVidImagePartial *image
    );
    void SetPos(
        int newX,
        int newY
    );
    void InvalidateRect(const HudUiRect *dirtyRect);
    virtual int GetCenterX();
    virtual int GetCenterY();
    int HitTest(
        int px,
        int py
    );
    virtual RECOIL_NO_GS void RebuildBltRectFromImage();
    void Shutdown();
    void Draw();
};

extern HudUiRect g_HudUiMgrSensor_FxRectScratch;

namespace HudUiLayoutNode {
int __fastcall ReadRect(
    zReader::Node *node,
    HudUiRect *outRect
);
int __fastcall ReadInt3(
    zReader::Node *node,
    int *out0,
    int *out1,
    int *out2
);
int __fastcall ReadRectOffsetAndSize(
    zReader::Node *node,
    HudUiRect *outRect,
    const int *offsetXY,
    int *outWidth,
    int *outHeight
);
int __fastcall ApplyTextLabel(
    zReader::Node *layoutNode,
    HudUiPanel *target,
    int baseX,
    int baseY,
    const int *offsetXY
);
zVidImagePartial *__fastcall ApplyImageWidget(
    zReader::Node *layoutNode,
    HudUiWidget *widget,
    int baseX,
    int baseY,
    const int *anchorOrNull,
    zVidImagePartial *preloadedImageOrNull,
    HudUiRect *outRectOrNull
);
int __fastcall ApplyCornerTextQuad(
    zReader::Node *node,
    HudUiBar *target,
    const int *offsetXY,
    HudUiRect *outRect
);
int __fastcall ApplyMeterQuad(
    zReader::Node *node,
    HudUiBar *target,
    int xBase,
    int yBase,
    const int *offsetXY,
    HudUiRect *outRect
);
} // namespace HudUiLayoutNode

struct HudUiMgrSensorBlock;

extern HudUiMgrSensorBlock g_HudUiMgrSensorBlock;
extern HudUiRect g_HudUiMgrSensorFxRect;
extern int g_HudUiMgrSensorFxViewportWidth;
extern int g_HudUiMgrSensorFxViewportHeight;
extern int g_HudUiMgrReticleSnapRadiusSq;

struct HudUiContainer {
    HudUiContainer();
    ~HudUiContainer();

    virtual void UpdateAll(float deltaSeconds);
    virtual void SetEnabled(int enabledValue);

    int enabled;
    HudUiElement *childHead;
    HudUiElement *childTail;

    void DestructorCore();
    int AddChild(HudUiElement *child);
    int FindChildWithPrev(
        HudUiElement *child,
        HudUiElement **previousOut
    );
    int RemoveChild(HudUiElement *child);
    void SetChildFlags(unsigned int childFlags);
    void InvalidateChildren();
};

struct HudLayoutBase : HudUiContainer {
    HudUiRect layoutRect;
    HudUiRect activeRect;
    HudUiWidget widget0;

    HudLayoutBase();
    static void Shutdown_Stub();
    void Destructor();
    virtual int SetActive(int active);
    virtual void UpdateAll(float deltaSeconds);
    virtual void LayoutPreUpdate();
    virtual void Enable();
    virtual void Disable();
    virtual void OnActivated();
    void LoadTypeIFromZarRoot(zReader::Node *parentNode);
};

struct HudLayoutSW : HudLayoutBase {
    HudLayoutSW();
    virtual int SetActive(int active);
};

struct HudLayoutHW : HudLayoutBase {
    HudUiWidget widget1;
    zVidImagePartial *widget1ImageDefault;
    zVidImagePartial *widget1Image320;
    zVidImagePartial *widget1Image400;
    HudUiWidget widget2;
    zVidImagePartial *widget2ImageDefault;
    zVidImagePartial *widget2Image320;
    zVidImagePartial *widget2Image400;
    HudUiWidget widget3;
    HudUiRect reticleClipRect;
    unsigned char reticleClipInitFlags;
    unsigned char unknown_349[0x03];

    HudLayoutHW();
    virtual void UpdateAll(float deltaSeconds);
    int LoadTypeIIFromZarRoot(zReader::Node *parentNode);
    void ReleaseImages();
    virtual int SetActive(int active);
    virtual void OnActivated();
    virtual void UpdateObjectiveDirtyRect();
    virtual void Enable();
    virtual void Disable();
};

extern HudLayoutSW g_HudLayoutSW;
extern HudLayoutHW g_HudLayoutHW;

extern HudUiTextStack4 *g_HudUiChatMessageStack;
extern HudUiTextStack4 *g_HudUiTopMessageStack;

extern HudUiShieldMessageWidget *g_HudUiMgrShieldMessageWidget;

namespace HudLayout {
int __fastcall ApplyViewportRect(HudUiRect *activeRect);
}

namespace HudUiMgrSensor {
void __cdecl TrackList_Reset();
HudUiMgrSensorTrackNode *__fastcall TrackList_Add(
    int trackKind,
    void *payload
);
int __fastcall PlaceTrackCounterWidget(
    HudUiMgrSensorTrackNode *trackNode,
    const zVec3 *worldPoint
);
int __fastcall PlaceTrackMarker(
    int markerMode,
    PlayerProgressTargetSlotRuntime *outputSlots
);
void __fastcall UpdateMarkersAndProgressFromVariantTag(
    const zTag4Partial *requiredVariantTag
);
void __fastcall SetShieldMessageRatio(float ratio);
void __fastcall SetViewportRect(
    int x,
    int y,
    int width,
    int height
);
void __fastcall GetFxRect(HudUiRect *outRect);
} // namespace HudUiMgrSensor

namespace HudUiMgr {
void __cdecl StaticInitAndRegisterAtExit();
HudUiContainer *StaticInit();
void RegisterAtExit();
void __cdecl AtExitDestructor();
void __fastcall StaticDestructor(HudUiContainer *self);
int __fastcall ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint,
    zVec3 *projectedPoint
);
void __fastcall ScreenToWorld(float *pointXY);
void __cdecl TriggerCurrentLayoutOnActivated();
int TickLayoutDelay();
int IsLocalPlayerFirstInStatsList();
void __fastcall SetNanitePanelCount(int count);
void __fastcall SetModeCounterState(
    int counterIndex,
    int state
);
void __cdecl ReticleStaticAtexitStub();
void __fastcall CopyReticleProjection(float *outProjection);
void __fastcall SetReticleMode(int mode);
int __fastcall EnsureHudLoaded(const char *entryPath);
int __fastcall UpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY
);
void __fastcall OnViewportChanged(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
);
void __fastcall ActivateHud(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
);
void DestroySensorWindow();
int EnableHud();
int DisableHud();
int ToggleHud();
void UpdateFrame();
void __fastcall SwitchActiveDialog(HudLayoutBase *newDialog);
int __fastcall ApplyHudModeSwitch(int hudType);
void __fastcall SetFloatTimerVisible(int visible);
void __fastcall HideTrackedProgressMeterIfOwnerMatches(void *ownerPayload);
void __fastcall SetAuxOverlayVisible(int visible);
void EnableTopAndChatStacks();
void DisableTopAndChatStacks();
int __fastcall InitHudLayouts(
    const HudUiRect *displaySection,
    const HudUiRect *windowSection
);
void ShutdownResources();
} // namespace HudUiMgr

namespace HudUiSensorWindow {
void __cdecl StaticInitAndRegisterAtExit();
} // namespace HudUiSensorWindow

namespace HudUiAuxOverlay {
void __fastcall UpdateTextLine(
    int op,
    int index,
    const char *format
);
void ClearTextLines();
} // namespace HudUiAuxOverlay

namespace HudUiLoadingCheckpoint {
void __fastcall AdvanceAndLog(const char *messageOrNull);
void InitTable();
} // namespace HudUiLoadingCheckpoint

namespace HudUi {
void __fastcall SetInvalidateMode(int mode);
int __fastcall ShowMessageBox(
    const char *messageText,
    const char *titleText,
    void *modalContext
);
void __fastcall HandleHotkeyCommand(int commandId);
void __fastcall ShowTopMessageLine(
    const char *message,
    float duration
);
void __fastcall ShowChatLine(
    const char *message,
    float duration
);
void __fastcall PushTopMessageLine(
    const char *message,
    float duration
);
void __fastcall PlayPowerupSfx(int shouldPlay);
void __fastcall RefreshScoreboardEntryRow(GameNetPlayerRow *entryData);
void __fastcall RemoveScoreboardEntryRow(GameNetPlayerRow *entryKey);
} // namespace HudUi

namespace HudScoreboard {
void __stdcall SetScaleAndRebuild(float scale);
void __stdcall DispatchSetScale(float deltaTime);
} // namespace HudScoreboard

extern char g_HudUiMessage_NodeName[8];
extern char g_HudUiMessage_SeparatorColon[2];
extern char g_HudSensorTracker_ReadFileFailedFmt[18];
extern char g_HudCfgKey_Fonts[6];
extern char g_HudZrd_Key_Sound[6];
extern int g_HudUiMgrObjectiveChatComposeActive;
struct HudUiObjectiveBar;
extern HudUiWidget g_HudUiMgrSensorPanel;
extern HudUiWidget g_HudUiMgrSensorOverlay;
extern HudUiManagerMeterCandidate g_HudUiMgrSensorMeter;
extern int g_HudUi_AuxOverlayEnabled;

namespace HudUiMgrObjective {
void __fastcall RefreshCounterText(int counterValue);
void __fastcall SetVisibleAndResetMeterFill(int visible);
void TickMeterFillAnimation();
void UpdateMeterXPoints();
int __fastcall Show(
    zVidImagePartial *objectiveImage,
    const char *summaryFormat,
    const char *descText,
    float autoHideDelay
);
void Begin();
void StartHide();
void Update();
} // namespace HudUiMgrObjective

namespace HudUiMgrTarget {
void __fastcall UpdateSelectedProgressMeter(int clearSelectedTrack);
} // namespace HudUiMgrTarget

struct StdPtrVector {
    unsigned char allocator;
    unsigned char padding_01[3];
    int *begin;
    int *end;
    int *capacityEnd;

    void ClearNoOpDestroy(
        int *begin,
        int *end
    );
};

struct HudUiPrimitiveBindTarget : HudUiElement {
    int endX;
    int endY;
    unsigned int color565;

    void SetSegmentEndpoints(
        int startX,
        int startY,
        int endX,
        int endY
    );
};

/**
 * Retail table evidence at 0x4d3c70 proves the 29-slot HudUiElement prefix
 * followed only by HudUiTextLabel::SetTextFmt.
 */
struct HudUiTextLabel : HudUiElement {
    char textBuffer[0x100];
    int fontHandle;
    int centerText;
    int centerBoundsLeft;
    int centerBoundsRight;
    int alignMode;

    /**
     * Original helper; no standalone retail function exists. Observed in the
     * HudUiTextLabel/HudUiPanel method cluster where 0x4ba850 constructs raw
     * panel storage before the 0x4bcbe0 text-label copy constructor rebuilds
     * the base state.
     * Purpose: keep caller-owned text-label storage from running the
     * address-backed 0x4bcb50 constructor implicitly.
     */
    HudUiTextLabel() {
    }
    HudUiTextLabel(
        const char *text,
        int x,
        int y,
        int flags
    );
    HudUiTextLabel(const HudUiTextLabel &source);
    HudUiTextLabel * ConstructorWithPosAndFlags(
        const char *text,
        int x,
        int y,
        int flags
    );
    HudUiTextLabel & operator=(const HudUiTextLabel &source);
    virtual void __cdecl SetTextFmt(
        const char *format,
        ...
    );
    void RebuildTextBounds();
    int MeasureTextWidth();
    void OnDraw();
    int HitTest(
        int px,
        int py
    );
    void UpdateTextExtents();
};

/**
 * HudUiCircle owner evidence: BN constructor 0x4bc480 proves a HudUiElement
 * base at offset zero plus circle radius/color fields. The retail
 * g_HudUiCircle_FTable pointer is dispatch/data evidence for the owner and
 * remains data/byte-verification debt, not a production FTable scaffold.
 */
struct HudUiCircle : HudUiElement {
    int radius;
    int radiusSquared;
    unsigned int color565;

    /**
     * Original inline helper; no standalone retail function exists. Observed
     * in owner storage that is explicitly address-constructed later.
     * Purpose: keep native raw-storage tests and aggregate owners from running
     * the address-backed circle constructor implicitly.
     */
    HudUiCircle() {
    }
    /**
     * Purpose: construct the circle HUD element and install its C++ dispatch
     * identity.
     */
    HudUiCircle(
        int x,
        int y,
        int circleRadius,
        unsigned int circleColor565
    );
    virtual void Draw();
    int HitTest(
        int px,
        int py
    );
    unsigned char HitTestCore(
        int px,
        int py
    );
};

struct HudUiBackgroundContainer : HudUiContainer {
    HudUiElement *inputFocusElement;
    zInput::MouseStateSnapshot mouseState;
    int captureTransitionMask;

    HudUiBackgroundContainer(int initFlag);
    ~HudUiBackgroundContainer();
    virtual void UpdateAll(float deltaSeconds);
    virtual void SetEnabled(int enabled);
    void SetInputFocus(HudUiElement *element);
    HudUiElement * GetInputFocus();
};

struct HudUiBackgroundSoundEntry {
    zSndSample *sample;
    float volume;
    zSndPlayHandle *playHandle;
};

struct HudUiDialogController {
    unsigned char reserved00[0x114];
    zVidImagePartial *capturedImage;

    void BlitOwnedSurfaceToPrimary();
};

struct HudUiBackgroundCursorWidget : HudUiWidget {
    zVidImagePartial *capturedImage;
    int captureEnabled;
    int captureSourceSelector;
    int reservedC8;
    int reservedCC;

    HudUiBackgroundCursorWidget(
        const char *imagePath,
        int captureEnabled
    );
    ~HudUiBackgroundCursorWidget();
    virtual void SetImageBorrowedAndRefreshIfChanged(zVidImagePartial *image);
    virtual void SetImageByPathOwnedAndRefresh(const char *imagePath);
    void SetImageOwnedAndRefresh(int captureEnabled);
    void SetImageBorrowedAndRefresh();
    void SetPos(
        int x,
        int y
    );
    void RebuildCapturedImage(
        int x,
        int y
    );
    void Draw();
    void DrawBase();
};

struct HudUiBackgroundVideoWidget : HudUiElement {
    zFMV_Stream *stream;
    float elapsedTimeSec;
    unsigned short colorKey565;
    char mediaPath[0x106];

    HudUiBackgroundVideoWidget();
    ~HudUiBackgroundVideoWidget();
    void Destructor();
    void SetMediaPathOwnedAndRefresh(const char *path);
    void SetColorKey565(unsigned short colorKey);
    void Update(float deltaSeconds);
    void Draw();
    void DrawBase();
    virtual void RebuildBltRect();
};

struct HudUiBackgroundMemberCursorWidget : HudUiBackgroundCursorWidget {
    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: observed in HudUiBackground construction at 0x4b9540 for the
     * cursorWidget member at offset 0x44.
     * Purpose: construct the background cursor member through the recovered base
     * cursor-widget constructor.
     */
    HudUiBackgroundMemberCursorWidget(
        const char *imagePath,
        int captureEnabled
    ) : HudUiBackgroundCursorWidget(
            imagePath,
            captureEnabled
        ) {
    }
};

/**
 * Compiler-emitted 0x4ba470: canonical VC5
 * std::vector<HudUiPanel *>::~vector provider COMDAT used by the four
 * HudUiZrdWidget panel-vector members. This is provider output from the
 * ordinary STL source model, not an authored retail owner.
 */
typedef std::vector<HudUiPanel *> HudUiPanelPtrVector;

struct HudUiZrdWidget : HudUiWidget {
    int originX;
    int originY;
    int modeOrEnabled;
    HudUiBackground *owner;
    HudUiRect boundsRect;
    zVidImagePartial *defaultImage;
    zVidImagePartial *disabledImage;
    zVidImagePartial *rolloverImage;
    zSndSample *rolloverSound;
    zSndPlayHandle *rolloverPlayHandle;
    float rolloverSoundScale;
    zVidImagePartial *activateImage;
    zSndSample *activateSound;
    zSndPlayHandle *activatePlayHandle;
    float activateSoundScale;
    zSndSample *disabledSound;
    float disabledSoundScale;
    HudUiPanelPtrVector labelPanels;
    HudUiPanelPtrVector rolloverLabelPanels;
    HudUiPanelPtrVector activateLabelPanels;
    HudUiPanelPtrVector disabledLabelPanels;

    HudUiZrdWidget();
    ~HudUiZrdWidget();
    HudUiZrdWidget * Constructor();
    void DestructorCore();
    void DestructorCoreThunk();
    void Invalidate();
    HudUiRect * GetBoundsRectOrNull();
    void ShowPreview();
    void OnActivate();
    virtual void RefreshState();
    virtual int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    virtual void PostLoadFromZrd();
    void HidePreview();
    static void *__stdcall DeleteChildIfPresent(void *childWidgetOrNull);
};

struct HudUiCheckToggleWidget : HudUiZrdWidget {
    int checked;
    zVidImagePartial *disabledCheckedImage;
    zVidImagePartial *disabledCheckedFallbackImage;
    zVidImagePartial *uncheckedImage;
    zVidImagePartial *checkedImage;
    HudUiPanel *checkedLabelPanel;

    HudUiCheckToggleWidget();
    ~HudUiCheckToggleWidget();
    void DestructorCore();
    void DestructorCoreThunk();
    HudUiRect * GetBoundsRectOrNull();
    void RefreshState();
    void ShowPreview();
    void HidePreview();
    void OnActivate();
    void OnActivateThunk();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    int SetChecked(int newChecked);
};

struct HudUiCycleSelectorWidget : HudUiZrdWidget {
    int selectedIndex;
    int itemCount;
    int firstIndex;
    int visibleCount;
    void *fontStyleRef;
    int textOffsetX;
    int textOffsetY;
    HudUiWidget *entriesA[20];
    HudUiWidget *entriesB[20];

    HudUiCycleSelectorWidget();
    ~HudUiCycleSelectorWidget();
    HudUiCycleSelectorWidget * Constructor();
    void DestructorCore();
    void DestructorCoreThunk();
    void AdvanceSelectionAndActivate();
    int SetIndexClamped(int index);
    void SetVisibleRange(
        int first,
        int last
    );
    void Update(float deltaSeconds);
    void AddTextEntry(
        int index,
        const char *text,
        int posX,
        int posY
    );
    void ApplyFontStyleForEntry(
        int index,
        int styleIndex
    );
    void AddBitmapEntry(
        int index,
        const char *imagePath,
        int posX,
        int posY
    );
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
};

struct HudUiFillBitmap : HudUiZrdWidget {
    float normalizedValue;
    zVidImagePartial *fillImage;
    HudUiRect fillRect;
    int fillOffsetX;
    int fillOffsetY;
    zVidImagePartial *previewImage;
    HudUiRect previewRect;
    int previewOffsetX;
    int previewOffsetY;

    HudUiFillBitmap();
    ~HudUiFillBitmap();
    void DestructorCore();
    void DestructorCoreThunk();
    void Draw();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void UpdateNormalizedFromCursor();
    void SetNormalizedValue(float value);
    virtual void SetNormalizedValueAndRebuild(float value);
};

struct HudUiZrdWidgetEx17C_Item : HudUiZrdWidget {
    int selected;
    HudUiZrdWidgetEx17C *ownerSelector;
    int itemIndex;
    int mouseRectValid;
    HudUiRect mouseRect;
    zVidImagePartial *selectedImage;
    zVidImagePartial *unselectedImage;
    zVidImagePartial *selectedRolloverImage;
    zVidImagePartial *unselectedRolloverImage;

    HudUiZrdWidgetEx17C_Item();
    HudUiZrdWidgetEx17C_Item * Constructor();
    void DestructorCore();
    void ShowPreviewIfNotSelected();
    void HidePreviewIfNotSelected();
    void ShowPreview();
    void HidePreview();
    void OnActivate();
    void OnActivateSelectSelf();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void SetSelected(int selectedValue);
    HudUiRect * GetMouseRectOrBounds();
};

struct HudUiZrdWidgetEx17C : HudUiZrdWidget {
    int optionCount;
    HudUiZrdWidgetEx17C_Item *options[10];
    int selectedIndex;

    HudUiZrdWidgetEx17C();
    ~HudUiZrdWidgetEx17C();
    HudUiZrdWidgetEx17C * Constructor();
    void DestructorCore();
    void SetVisible(int childIndex);
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void EnableChildAtIndex(int childIndex);
    int SetSelectedIndex(int index);
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-panel.type
 * @recoil-artifact emits .text recoil:function:0x40a590: VC5 scalar deleting destructor for this virtual-destructor model.
 * Retail table evidence at 0x4d3a88 proves the SetTextFmt override and the
 * following seven Panel-introduced virtuals in declaration order.
 */
struct HudUiPanel : HudUiTextLabel {
    zVidImagePartial *textPick;
    unsigned int textColor0;
    unsigned int textColor1;
    HGDIOBJ hFont;
    unsigned int cachedTextLength;
    char cachedText[0x100];
    int textWidthPx;
    int textHeightPx;
    unsigned int shadowEnabled;
    int bkMode;
    COLORREF bkColor;
    unsigned int textDirty;
    int unknown274;
    unsigned int wordWrapEnabled;
    HudUiRect wrapRect;
    HudUiRect textRect;
    int shadowOffsetX;
    int shadowOffsetY;

    HudUiPanel(
        const char *text = 0,
        int x = 0,
        int y = 0
    );
    HudUiPanel(const HudUiPanel &source);
    ~HudUiPanel();
    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: recovered in the HUD source cluster near address-backed
     * 0x4bd100 HudUiPanel::ConstructorDefaultThunk callers.
     * Purpose: preserve the recovered HUD behavior for
     * HudUiPanel::ConstructorDefault.
     */
    HudUiPanel * ConstructorDefault(
        const char *text,
        int x,
        int y
    ) {
        new (this) HudUiPanel(
            text,
            x,
            y
        );
        return this;
    }
    HudUiPanel * ConstructorDefaultThunk();
    HudUiPanel & operator=(const HudUiPanel &source);
    void SetClip(
        void *bltSourceOrNull,
        const HudUiRect *rectOrNull
    );
    void Invalidate();
    virtual void __cdecl SetTextFmt(
        const char *format,
        ...
    );
    virtual void UpdateTextBoundsFromContent();
    virtual HGDIOBJ GetFont();
    virtual void SetFont(
        const char *faceName,
        int height,
        int weight,
        int width,
        int italic,
        int charSet,
        int pitchAndFamily
    );
    virtual void SetFontHandle(HGDIOBJ fontHandle);
    virtual void SetTextFmtV(
        const char *format,
        va_list args
    );
    virtual void SetText(const char *text);
    virtual void RebuildTextRect();
    void EnableWordWrapWithRect(const HudUiRect *rect);
    void Draw();
    unsigned int SetTextColor(unsigned int color);
    void SetTextColorsAndMarkDirty(
        unsigned int color0,
        unsigned int color1
    );
    unsigned int SetShadow(
        unsigned int shadowEnabled,
        int shadowOffsetX,
        int shadowOffsetY
    );
    int MeasureTextPrefixRect(
        int maxChars,
        RECT *outRect
    );
    int QueryTextHeight();
    HudUiRect * GetWrapRect();
    int HitTest(
        int px,
        int py
    );
    char * GetLastTextPtr();
    void GetTextRect(HudUiRect *outRect);
};

struct HudUiListSelectorItem : HudUiPanel {
    int entryIndex;
    void *owner;

    /**
     * Purpose: construct an empty list-selector entry over the panel base.
     */
    HudUiListSelectorItem() :
        HudUiPanel(
            0,
            0,
            0
        )
    {
    }
    void OnActivate();
    void Draw();
};

/*
 * HudCmd binding owner evidence: BN function 0x40be00 destroys only the
 * display-text field at offset zero, while command-list entries extend that
 * prefix with the command id consumed by HudCmdDialog selection logic.
 */
struct HudCmdBinding {
    char *displayText;
};

struct HudCmdBindingEntry;

#if !defined(_MSC_VER) || _MSC_VER >= 1200
struct HudCmdBindingVector {
    unsigned char allocator;
    unsigned char padding_01[3];
    HudCmdBindingEntry **first;
    HudCmdBindingEntry **last;
    HudCmdBindingEntry **limit;

    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindingVector.
 */
HudCmdBindingVector() {
#if defined(_MSC_VER) && _MSC_VER < 1200
        char allocatorValue;
#else
        char allocatorValue = 0;
#endif
        allocator = (unsigned char)(allocatorValue);
        first = 0;
        last = 0;
        limit = 0;
    }

    /**
     * Original inline member-lifetime evidence: the complete command-binding
     * button destructors release the vector allocation after authored entry
     * cleanup and before the embedded panel and widget base are destroyed.
     * Purpose: release the command-binding pointer-vector storage.
     */
    ~HudCmdBindingVector() {
        ::operator delete(first);
        first = 0;
        last = 0;
        limit = 0;
    }

    /**
     * Original-source helper; no standalone retail function exists.
     * Purpose: return the first mutable pointer-vector slot.
     */
    HudCmdBindingEntry **begin() { return first; }
    /**
     * Original-source helper; no standalone retail function exists.
     * Purpose: return the mutable pointer-vector end iterator.
     */
    HudCmdBindingEntry **end() { return last; }
    /**
     * Original-source helper; no standalone retail function exists.
     * Purpose: return the first const pointer-vector slot.
     */
    HudCmdBindingEntry *const *begin() const { return first; }
    /**
     * Original-source helper; no standalone retail function exists.
     * Purpose: return the const pointer-vector end iterator.
     */
    HudCmdBindingEntry *const *end() const { return last; }
    unsigned int size() const;
    HudCmdBindingEntry **erase(
        HudCmdBindingEntry **eraseFirst,
        HudCmdBindingEntry **eraseLast
    );
    void push_back(HudCmdBindingEntry *entry);
};
#endif

struct HudCmdBindingEntry : HudCmdBinding {
    int commandId;

    /**
     * No standalone retail function; this preserves the empty record
     * constructor used by existing source and test fixtures that assign the
     * two fields explicitly.
     * Purpose: provide default construction for command-binding entry records.
     */
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindingEntry.
 */
HudCmdBindingEntry() {
    }

    /**
     * No standalone retail function; Binary Ninja shows the constructor body
     * inlined in HudCmdBindButtonBase::AddBindingEntry at 0x40bf80, where the
     * allocated entry receives _strdup(text) at offset 0 and id at offset 4.
     * Purpose: initialize one command-binding display entry.
     */
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindingEntry.
 */
    HudCmdBindingEntry(
        const char *text,
        int id
    )
    {
        displayText = _strdup(text);
        commandId = id;
    }

    /**
     * Binary Ninja shows the body destroying only the offset-zero display
     * string owned by the binding entry.
     * Purpose: release the entry-owned display string before scalar delete.
     */
    ~HudCmdBindingEntry();

};

/**
 * Provider boundary 0x40be00: canonical VC5 std::transform specialization.
 * through that canonical provider instantiation rather
 * than an authored replacement body.
 * The empty source functor deletes one entry and returns null, producing the
 * retail delete-and-null transform body without a hand-authored provider.
 * Purpose: express command-binding entry cleanup through ordinary STL source.
 * legacy verification anchor now represented only by
 * natural compiler output from this functor/lifetime shape; its exact later
 * full-order classification remains a parent-owned Binary Ninja decision.
 */
struct HudCmdBindingEntryDelete {
    /**
     * Purpose: declare the deleting functor used by command-binding cleanup.
     */
    HudCmdBindingEntry *operator()(HudCmdBindingEntry *entry) const;
};

/**
 * No standalone retail function for this HudCmd vector; Binary Ninja shows
 * HudCmdBindButtonBase::AddBindingEntry at 0x40bf80 inlining the previous
 * entry count as (end - begin), and zInput_BindGroupInfoVec::Count at 0x42a9d0
 * proves the same VC pointer-vector count idiom for this codebase.
 * Purpose: return the number of command-binding entries currently stored.
 */
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindingVector::Count.
 */
#if !defined(_MSC_VER) || _MSC_VER >= 1200
/**
 * Original-source helper; no standalone retail function exists.
 * Purpose: return the compatibility pointer-vector element count.
 */
inline unsigned int HudCmdBindingVector::size() const {
    if (first == 0) {
        return 0;
    }

    return (unsigned int)(last - first);
}

/**
 * No standalone retail function; Binary Ninja shows
 * HudCmdBindButtonBase::AddBindingEntry at 0x40bf80 inlining a VC
 * pointer-vector append/growth path over bindingVec, including capacity
 * growth, pointer copy, and old storage release when the buffer is full.
 * Purpose: append one command-binding entry while preserving vector storage.
 */
/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindingVector::PushBack.
 */
inline void HudCmdBindingVector::push_back(
    HudCmdBindingEntry *entry
) {
    HudCmdBindingEntry **const insertPos = last;
    HudCmdBindingEntry *value = entry;
    if ((unsigned int)(limit - insertPos) < 1u) {
        const int currentCount = (int)size();
        const int growBy = currentCount > 1 ? currentCount : 1;
        const int newCapacityCount = currentCount + growBy;
        HudCmdBindingEntry **const newBegin =
            (HudCmdBindingEntry **)(::operator new(
                (unsigned int)newCapacityCount * sizeof(HudCmdBindingEntry *)
            ));
        HudCmdBindingEntry **write = newBegin;
        HudCmdBindingEntry **read = first;
        while (read != insertPos) {
            *write = *read;
            ++read;
            ++write;
        }

        *write = value;
        ::operator delete(first);
        first = newBegin;
        last = newBegin + currentCount + 1;
        limit = newBegin + newCapacityCount;
        return;
    }

    *insertPos = value;
    last = insertPos + 1;
}
#endif

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-cmd-bind-button-base.type
 * @recoil-artifact emits .text recoil:function:0x40c260: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * HudCmd bind buttons are authored C++ UI classes: BN table evidence places
 * OnSelectionChangedRefresh in the final generated vtable slot after
 * HudUiZrdWidget::PostLoadFromZrd, so keep it virtual instead of modeling a
 * copied FTable.
 * Purpose: Record the compiler-generated lifecycle code emitted by the complete bind-button base type.
 */
struct HudCmdBindButtonBase : HudUiCheckToggleWidget {
    int bindingSlotTotalCount;
    int visibleBindingSlotCount;
    HudUiListSelectorItem bindPanel;
    HudUiListSelectorItem *bindingSlotPanels;
#if defined(_MSC_VER) && _MSC_VER < 1200
    std::vector<HudCmdBindingEntry *> bindingVec;
#else
    HudCmdBindingVector bindingVec;
#endif
    int bindingSlotSpacing;
    int selectedBindingIndex;
    float visibleListOffsetX;
    float visibleListOffsetY;
    float overflowListOffsetX;
    float overflowListOffsetY;
    int selectedFontStyleRef;
    int listFontStyleRef;

    HudCmdBindButtonBase();
    /**
     * Purpose: run the optimizer-visible entry cleanup before ordinary vector,
     * panel, and widget-base lifetime teardown.
     */
    virtual ~HudCmdBindButtonBase();
    int AddBindingEntry(
        const char *displayText,
        int commandId
    );
    void OnSelectedIndexChanged(int selectedIndex);
    void SetSelectedEntry(int selectedIndex);
    /**
     * Final HudCmd bind-button vtable slot at 0x84 in BN.
     */
    virtual void OnSelectionChangedRefresh(int selectedIndex);
    /**
     * Provider boundary 0x40be60: canonical VC5 std::copy specialization
     * selected by vector::clear().
     * through that canonical provider instantiation.
     * Purpose: delete and null every owned entry, then clear the pointer range.
     */
    void ClearBindingEntries();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void RebuildBindingSlotWidgets(
        int totalCount,
        int visibleCount
    );
};

struct HudCmdCommandList : HudCmdBindButtonBase {

    /**
     * Purpose: preserve the natural complete destructor generated for this
     * concrete command-list member lifetime.
     */
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * command-list construction as HudCmdBindButtonBase construction followed
     * by compiler-generated table emission.
     * Purpose: construct the command-list bind-button subobject.
    */
    HudCmdCommandList() : HudCmdBindButtonBase() {}
    /**
     * Purpose: preserve the natural implicit lifecycle while ordinary C++
     * rules destroy the common bind-button base.
     */
};

struct HudCmdKeyAButton : HudCmdBindButtonBase {

    /**
     * Purpose: preserve the natural complete destructor generated for this
     * concrete primary-key member lifetime.
     */
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * key-A button construction as HudCmdBindButtonBase construction followed
     * by compiler-generated table emission.
     * Purpose: construct the primary-key bind-button subobject.
    */
    HudCmdKeyAButton() : HudCmdBindButtonBase() {}
    /**
     * Purpose: preserve the natural implicit lifecycle while ordinary C++
     * rules destroy the common bind-button base.
     */
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdKeyBButton : HudCmdBindButtonBase {

    /**
     * Purpose: preserve the natural complete destructor generated for this
     * concrete secondary-key member lifetime.
     */
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * key-B button construction as HudCmdBindButtonBase construction followed
     * by compiler-generated table emission.
     * Purpose: construct the secondary-key bind-button subobject.
    */
    HudCmdKeyBButton() : HudCmdBindButtonBase() {}
    /**
     * Purpose: preserve the natural implicit lifecycle while ordinary C++
     * rules destroy the common bind-button base.
     */
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdJoyButton : HudCmdBindButtonBase {

    /**
     * Purpose: preserve the natural complete destructor generated for this
     * concrete joystick member lifetime.
     */
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * joystick button construction as HudCmdBindButtonBase construction
     * followed by compiler-generated table emission.
     * Purpose: construct the joystick bind-button subobject.
    */
    HudCmdJoyButton() : HudCmdBindButtonBase() {}
    /**
     * Purpose: preserve the natural implicit lifecycle while ordinary C++
     * rules destroy the common bind-button base.
     */
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdMouseButton : HudCmdBindButtonBase {

    /**
     * Purpose: preserve the natural complete destructor generated for this
     * concrete mouse member lifetime.
     */
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * mouse button construction as HudCmdBindButtonBase construction followed
     * by compiler-generated table emission.
     * Purpose: construct the mouse bind-button subobject.
    */
    HudCmdMouseButton() : HudCmdBindButtonBase() {}
    /**
     * Purpose: preserve the natural implicit lifecycle while ordinary C++
     * rules destroy the common bind-button base.
     */
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudUiMessageBoxDialog;

struct HudUiMessageBoxOkButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudUiMessageBoxCancelButton : HudUiZrdWidget {

    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-triplet-panel.type
 * @recoil-artifact emits .text recoil:function:0x40f2b0: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudUiTripletPanel type.
 */
struct HudUiTripletPanel : HudUiElement {
    int visibleCount;
    unsigned char unknown_38[0x04];
    HudUiWidget items[3];

    HudUiTripletPanel();
    void Draw();
    void SetVisibleCount(int count);
    void ShutdownItems_Stub();
    void DestructorCore();
    void UnwindDestructFirstItem();
};

struct HudUiNanitePanel : HudUiTripletPanel {
    void InitLayout(zReader::Node *layoutRoot);
};

struct HudUiPanelFull : HudUiPanel {
    int activeSideIndex;
    int layoutX;
    int layoutY;
};

struct HudUiPanelFontParams {
    const char *faceName;
    int height;
    int weight;
    int width;
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-message.type
 * @recoil-artifact emits .text recoil:function:0x40daa0: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudUiMessage type.
 */
struct HudUiMessage : HudUiWidget {
    zVidImagePartial *variantImages[5];
    zVidImagePartial *activeSideImages[2];
    zVidImagePartial *sideImageSwaps[2];
    HudUiPanelFull panel;
    HudUiWidget widget;

    HudUiMessage();
    ~HudUiMessage();
    /**
     * Purpose: preserve compatibility callers while routing through the true
     * C++ destructor used by HudUiMgrData member arrays.
     */
    void Destructor() {
        this->~HudUiMessage();
    }
    void Draw();
    void ReleaseImages();
    void RebuildWeaponLayout();
    int LoadWeaponLayoutFromNode(
        zReader::Node *layoutNode,
        const HudUiPanelFontParams *fontParams
    );
    static void __fastcall SelectVariantDisplay(
        int messageIndex,
        int variantIndex
    );
    static void __fastcall ApplySideImageSwap(
        int messageIndex,
        int sideIndex
    );
    static void __fastcall ClearDisplay(int messageIndex);
    static void __fastcall SetValueIfOwnerMatches(
        int messageIndex,
        int ownerSideIndex,
        float valueOrClearToken
    );
    static void __fastcall UpdateSelectedWeaponDisplay(
        int weaponBankIndex,
        int weaponSideIndex,
        float valueOrClearToken
    );
};

struct HudUiBarPoint {
    float x;
    float y;
    int reserved;
};

struct HudUiPolylinePoint {
    int x;
    int y;
};

struct HudUiPolyline : HudUiElement {
    HudUiPolylinePoint points[21];
    int pointCount;
    int color565;
    const RECT *clipRect;

    HudUiPolyline();
    HudUiPolyline * Constructor();
    void Draw();
    void SetPoint(
        int index,
        int x,
        int y
    );
};

struct HudUiSliderBorder : HudUiPolyline {
    int originX;
    int originY;
    int halfWidth;
    int height;
    int blinkEnabled;
    float blinkPeriodSec;
    float blinkTimeRemainingSec;
    int blinkDirSign;
    int caretHalfWidth;
    int inputActive;
    char sliderVisibleWhenInputActive;
    char rawKeyFilterEnabled;
    char unknown112[2];

    HudUiSliderBorder();
    HudUiSliderBorder * Constructor();
    void Update(float deltaSeconds);
    void SetBounds(
        int originX,
        int originY,
        int halfWidth,
        int height
    );
};

struct HudUiCounter : HudUiWidget {
    zVidImagePartial *stateImages[3];
    HudUiRect clipViewportRect;
    int layoutX;
    int layoutY;

    HudUiCounter();
    int ApplyFromLayoutNode(zReader::Node *layoutNode);
    void ReleaseStateImages();
    void UpdateLayoutPosition();
};

struct HudUiBar : HudUiElement {
    HudUiBarPoint points[21];
    int drawVertexCount;
    union {
        struct {
            int drawParam;
            int quadHeight;
            float quadLeftX;
        };
        struct {
            unsigned int color565;
            int fillPixelsMax;
            unsigned int meterFlags;
        };
        struct {
            unsigned int objectiveDrawParam;
            float slideRangeX;
            unsigned int chatComposeActive;
        };
    };

    HudUiBar();
    void Draw();
    void SetPointXY(
        int pointIndex,
        float x,
        float y
    );
};

/**
 * Provisional manager-meter base for retail constructor 0x40d9e0. Retail
 * manager construction calls this base twice before installing the same
 * most-derived manager-meter table for its two embedded leaves.
 */
struct HudUiManagerMeterBaseCandidate : HudUiBar {
    HudUiManagerMeterBaseCandidate();
};

struct HudUiManagerMeterCandidate : HudUiManagerMeterBaseCandidate {};

/**
 * Provisional shield-meter sibling for retail constructor 0x40fb70. Its
 * direct HudUiBar construction is distinct from the manager-meter branch.
 */
struct HudUiShieldMeterCandidate : HudUiBar {
    HudUiShieldMeterCandidate();
    ~HudUiShieldMeterCandidate() {}
};

struct HudUiObjectiveBar : HudUiBar {};

struct HudUiMgrSensorBlock {
    int state;
    HudUiRect sensorRectScaled;
    HudUiRect sensorRectRaw;
    HudUiRect sensorViewportRect;
    zClipAltFloatRect sensorPiVSrcRect;
    float sensorClampHalfW;
    float sensorClampHalfH;
    float sensorParam;
    float sensorRangeSq;
    HudUiSlot *trackedProgressSlot;
    HudUiWidget sensorPanel;
    HudUiWidget sensorOverlay;
    HudUiManagerMeterCandidate sensorMeter;
    HudUiShieldMessageWidget *shieldMessageWidget;
    HudUiStringMenu *stringMenu;
    zVidImagePartial *targetMarkerImages[5];
    int targetMarkerCount;

    HudUiMgrSensorBlock() : sensorPanel(0), sensorOverlay(0) {}
    ~HudUiMgrSensorBlock();
    /**
     * Purpose: preserve compatibility callers while routing through the true
     * C++ destructor used by HudUiMgrData.
     */
    void Destructor() {
        this->~HudUiMgrSensorBlock();
    }
};

struct HudUiTextInput {
    virtual void OnPrintableKey(int key);
    /**
     * Purpose: ignore a text-input key action without changing the buffer.
     */
    virtual void OnIgnoredKey(int key) {}
    virtual void OnAccept();
    virtual void OnCancel();
    virtual void OnBackspace();
    virtual void OnDeleteForward();
    virtual void OnMoveCursorLeft();
    virtual void OnMoveCursorRight();
    virtual void OnOverflow();

    char *buffer;
    int capacity;
    unsigned int cursor;
    char keyActionMap[0x100];

    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiTextInput.
 */
HudUiTextInput() {
    }
    ~HudUiTextInput();
    HudUiTextInput(int bufferSize);
    HudUiTextInput * Constructor(int bufferSize);
    void AllocTextBuffer(int bufferSize);
    void DestructorCore();
    void SetContents(const char *source);
    char * GetBuffer();
    void SetCursorPosition(int position);
    void DispatchKeyAction(int key);
    void InsertCharAtCursor(int ch);
    void BackspaceDeleteChar();
    void DeleteCharForward();
    void MoveCursorLeft();
    void MoveCursorRight();
    int ShiftTextRight(
        int count,
        int startPos
    );
    int ShiftTextLeft(
        int count,
        int startPos
    );
};

struct HudUiNumericTextInput;

struct HudUiOwnedTextInput : HudUiTextInput {
    HudUiNumericTextInput *owner;

    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOwnedTextInput.
 */
HudUiOwnedTextInput() {
    }
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOwnedTextInput.
 */
HudUiOwnedTextInput(int bufferSize) : HudUiTextInput(bufferSize),
        owner(0) {
    }
    virtual void OnAccept();
};

struct HudUiChatComposeTextInput : HudUiTextInput {
    HudUiChatComposeTextInput() : HudUiTextInput(256) {}
    virtual void OnAccept();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-numeric-text-input.type
 * @recoil-artifact emits .text recoil:function:0x41a3f0: Compiler-generated HudUiNumericTextInput destructor tail thunk.
 *
 * Retail evidence: the complete body is a five-byte tail jump to the authored
 * destructor at 0x4b4ac0. It has lifecycle/EH cleanup callers and no table or
 * data references, so it is compiler inventory rather than a second authored
 * destructor definition.
 */
struct HudUiNumericTextInput : HudUiZrdWidget {
    HudUiOwnedTextInput textInput;
    HudUiSliderBorder sliderBorder;

    HudUiNumericTextInput();
    ~HudUiNumericTextInput();
    HudUiNumericTextInput * Constructor(unsigned int maxDigits);
    HudUiNumericTextInput * BaseConstructor();
    void Destructor();
    void AllocTextBuffer(unsigned int bufferSize);
    char * GetBuffer();
    void Update(const char *text);
    RECOIL_NO_GS void UpdateCaptureUiAndClip(float deltaSeconds);
    int SetInputActive(int active);
    void SetRawKeyboardCapture(int enable);
    virtual int OnRawKeyboardChar(int key);
    virtual int OnAcceptForwardToCommit();
    virtual int CommitAndGetValue();
    void OnActivate();
    static int __fastcall RawKeyboardCallback(
        int key,
        HudUiNumericTextInput *callbackCtx
    );
};

struct HudUiNetGameSetupTextInput : HudUiNumericTextInput {
    void OnActivate();
    void OnActivateFocusAndCursor();
};

struct HudUiNetGameSetupOverlayOwner : RecoilStateDialogHost {
    int m_reconfigureExistingSession;

    HudUiNetGameSetupOverlayOwner();
    static void __cdecl StaticInitAndRegisterAtExit();
    static HudUiNetGameSetupOverlayOwner *__cdecl StaticInit();
    static void __cdecl RegisterAtExit();
    static void __cdecl AtExitDestructor();
    ~HudUiNetGameSetupOverlayOwner();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    static void __fastcall QueueEnterWithReconfigureFlag(int reconfigureExistingSession);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupOverlayOwner) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupOverlayOwner,
        m_dialog
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupOverlayOwner,
        m_reconfigureExistingSession
    ) == 0x08
);

union HudUiNetGameSetupOverlayOwnerStorage {
    unsigned long align;
    unsigned char bytes[sizeof(HudUiNetGameSetupOverlayOwner)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupOverlayOwnerStorage) == 0x0c);

#define g_HudUiNetGameSetupOverlayOwner \
    (*(HudUiNetGameSetupOverlayOwner *)&g_HudUiNetGameSetupOverlayOwner)

struct HudUiClampedIntTextInput : HudUiNumericTextInput {
    int minValue;
    int maxValue;

    HudUiClampedIntTextInput(unsigned int maxDigits);
    int OnRawKeyboardChar(int key);
    int CommitAndGetValue();
};

struct HudUiClampedIntStepButton : HudUiZrdWidget {
    HudUiClampedIntTextInput *targetInput;
    int stepDelta;

    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-slot.type
 * @recoil-artifact emits .text recoil:function:0x40dbd0: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * BN constructors at 0x40db20 initialize HudUiElement at object offset zero,
 * then construct the slot and marker widgets at offsets 0x48 and 0x104.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudUiSlot type.
 */
struct HudUiSlot : HudUiElement {
    unsigned int screenEdgeCode;
    void *trackNode;
    float screenX;
    float screenY;
    unsigned char unknown_44[0x04];
    HudUiWidget slotWidget;
    HudUiWidget trackMarkerWidget;

    HudUiSlot();
    ~HudUiSlot();
    HudUiSlot * Constructor();
    /**
     * Purpose: preserve compatibility callers while routing through the true
     * C++ destructor used by HudUiMgrData weaponSlots.
     */
    void Destructor() {
        this->~HudUiSlot();
    }
    void Draw();
};

struct HudUiMgrObjectiveBlock {
    unsigned int state;
    unsigned int phase;
    float phaseTimerSec;
    float phaseDurationSec;
    float autoHideDelaySec;
    unsigned int showResetUnused;
    int objectiveWidgetRightX;
    HudUiWidget objectiveWidget;
    HudUiWidget objectiveSensorRect;
    HudUiPanel *objectiveSummaryTextPanel;
    HudUiPanel *objectiveLabelTextPanel;
    HudUiManagerMeterCandidate objectiveMeter;
    float objectiveMeterFillAnimTimerSec;
    unsigned int objectiveMeterFillAnimEnabled;
    HudUiPanel *objectiveDescTextPanel;
    HudUiObjectiveBar objectiveBar;
    HudUiChatComposeTextInput chatComposeTextInput;
    HudUiCounterTextPanel *counterTextPanel;

    HudUiMgrObjectiveBlock() : objectiveWidget(0), objectiveSensorRect(0) {}
    ~HudUiMgrObjectiveBlock();
};

struct HudUiTransitionTextPanel : HudUiPanel {
    float flashCountdown;
    float flashResetValue;
    int flashAltColor0;
    int flashAltColor1;
    int flashEnabled;
    int flashMode;
    int flashDirectionSign;

    HudUiTransitionTextPanel();
    ~HudUiTransitionTextPanel();
    void Update(float deltaSeconds);
    void ResetFlashState(float flashRate);
    void SetFlashRate(float flashRate);
    void SetFlashColorAndRate(
        unsigned int flashColor,
        float flashRate
    );
};

struct HudUiMgrReticleMapCache {
    float scaleHalfH;
    int projectedX;
    int projectedY;
};

struct HudUiMgrMessageSelectionState {
    int activeWeaponMessageIndex;
    int activeWeaponSideIndex;
};

struct HudLoadingCheckpointTable {
    unsigned int unknown_00;
    float checkpointProgressRaw[25];
    float checkpointProgressNormalized[25];
    unsigned int maxCheckpointIndex;
    unsigned int currentCheckpointIndex;
    unsigned int unknown_D4;
    float currentProgress;
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-mgr-data.type
 * @recoil-artifact emits .text recoil:function:0x40d740: VC5 EH array-destructor cleanup helper for weaponSlots[32].
 * @recoil-artifact emits .text recoil:function:0x40d760: VC5 EH array-destructor cleanup helper for modeCounters[4].
 * @recoil-artifact emits .text recoil:function:0x40db00: VC5 EH array-destructor cleanup helper for messages[10].
 * BN models g_HudUiMgr as one zero-initialized object at 0x4e5ed0. This
 * recovered owner covers the typed object through tailBar at 0x7844. The
 * four-byte zero gap before g_HudLayoutHW is not part of the typed object.
 * Purpose: Record the member-array cleanup helpers emitted by the complete HudUiMgrData type.
 */
struct HudUiMgrData : HudUiContainer {
    int hudLayoutsInitialized;
    unsigned int hudLoaded;
    HudLayoutBase *currentLayout;
    HudUiTransitionTextPanel hudRootPanel;
    unsigned int unknown_2DC[4];
    int layoutDelayFrames;
    HudUiRect hudRect;
    float hudRectW;
    float hudRectH;
    HudUiRect viewRect;
    unsigned int viewRectW;
    unsigned int viewRectH;
    int hudOriginX;
    int hudOriginY;
    float reticleProjection[3];
    int reticleWidgetHalfW;
    int reticleWidgetHalfH;
    float reticleMapBiasX;
    float reticleMapBiasY;
    float reticleMapScaleHalfW;
    HudUiMgrReticleMapCache reticleMapProject;
    zVidImagePartial *reticleImages[3];
    int reticleMode;
    HudUiWidget reticleWidget;
    HudUiNanitePanel nanitePanel;
    HudUiMgrObjectiveBlock objective;
    HudUiMgrSensorBlock sensor;
    HudUiSlot weaponSlots[32];
    unsigned int reticleSnapRadiusSq;
    unsigned int weaponState;
    unsigned int unknown_4704[32];
    HudUiTimerPanel *timerPanel;
    HudUiTimerPanelFloat *timerPanelFloat;
    unsigned int activeModeCounterIndex;
    HudUiCounter modeCounters[4];
    HudUiMgrMessageSelectionState activeMessageSelection;
    HudUiMessage messages[10];
    HudUiStatsListElement *statsList;
    unsigned int statsListState1;
    unsigned int statsListState2;
    unsigned int statsListState3;
    unsigned int statsListState4;
    unsigned int statsListState5;
    HudLoadingCheckpointTable loadingCheckpointTable;
    HudUiBar tailBar;

    HudUiMgrData();
    ~HudUiMgrData();
};

#define g_HudUiMgrHudLayoutsInitialized (g_HudUiMgr.hudLayoutsInitialized)
#define g_HudUiMgrHudLoaded (g_HudUiMgr.hudLoaded)
#define g_HudUiMgrCurrentLayout (g_HudUiMgr.currentLayout)
#define g_HudUiMgrHudRootPanel (g_HudUiMgr.hudRootPanel)
#define g_HudUiMgrLayoutDelayFrames (g_HudUiMgr.layoutDelayFrames)
#define g_HudUiMgrHudRect (g_HudUiMgr.hudRect)
#define g_HudUiMgrHudRectW (g_HudUiMgr.hudRectW)
#define g_HudUiMgrHudRectH (g_HudUiMgr.hudRectH)
#define g_HudUiMgrViewRect (g_HudUiMgr.viewRect)
#define g_HudUiMgrHudOriginX (g_HudUiMgr.hudOriginX)
#define g_HudUiMgrHudOriginY (g_HudUiMgr.hudOriginY)
#define g_HudUiMgrReticleProjection (g_HudUiMgr.reticleProjection)
#define g_HudUiMgrReticleWidgetHalfW (g_HudUiMgr.reticleWidgetHalfW)
#define g_HudUiMgrReticleWidgetHalfH (g_HudUiMgr.reticleWidgetHalfH)
#define g_HudUiMgrReticleMapBiasX (g_HudUiMgr.reticleMapBiasX)
#define g_HudUiMgrReticleMapBiasY (g_HudUiMgr.reticleMapBiasY)
#define g_HudUiMgrReticleMapScaleHalfW (g_HudUiMgr.reticleMapScaleHalfW)
#define g_HudUiMgrReticleMapScaleHalfH (g_HudUiMgr.reticleMapProject.scaleHalfH)
#define g_HudUiMgrReticleProjectedX (g_HudUiMgr.reticleMapProject.projectedX)
#define g_HudUiMgrReticleProjectedY (g_HudUiMgr.reticleMapProject.projectedY)
#define g_HudUiMgrReticleImages (g_HudUiMgr.reticleImages)
#define g_HudUiMgrReticleMode (g_HudUiMgr.reticleMode)
#define g_HudUiMgrReticleWidget (g_HudUiMgr.reticleWidget)
#define g_HudUiMgrNanitePanel (g_HudUiMgr.nanitePanel)
#define g_HudUiMgrObjectiveState (g_HudUiMgr.objective.state)
#define g_HudUiMgrObjectivePhase (g_HudUiMgr.objective.phase)
#define g_HudUiMgrObjectivePhaseTimerSec (g_HudUiMgr.objective.phaseTimerSec)
#define g_HudUiMgrObjectivePhaseDurationSec (g_HudUiMgr.objective.phaseDurationSec)
#define g_HudUiMgrObjectiveAutoHideDelaySec (g_HudUiMgr.objective.autoHideDelaySec)
#define g_HudUiMgrObjectiveShowResetUnused (g_HudUiMgr.objective.showResetUnused)
#define g_HudUiMgrObjectiveWidgetRightX (g_HudUiMgr.objective.objectiveWidgetRightX)
#define g_HudUiMgrObjectiveWidget (g_HudUiMgr.objective.objectiveWidget)
#define g_HudUiMgrObjectiveSensorRect (g_HudUiMgr.objective.objectiveSensorRect)
#define g_HudUiMgrObjectiveSummaryTextPanel (g_HudUiMgr.objective.objectiveSummaryTextPanel)
#define g_HudUiMgrObjectiveLabelTextPanel (g_HudUiMgr.objective.objectiveLabelTextPanel)
#define g_HudUiMgrObjectiveMeter (g_HudUiMgr.objective.objectiveMeter)
#define g_HudUiMgrObjectiveMeterFillAnimTimerSec (g_HudUiMgr.objective.objectiveMeterFillAnimTimerSec)
#define g_HudUiMgrObjectiveMeterFillAnimEnabled (g_HudUiMgr.objective.objectiveMeterFillAnimEnabled)
#define g_HudUiMgrObjectiveDescTextPanel (g_HudUiMgr.objective.objectiveDescTextPanel)
#define g_HudUiMgrObjectiveBar (g_HudUiMgr.objective.objectiveBar)
#define g_HudUiMgrObjectiveChatComposeActive \
    (g_HudUiMgr.objective.objectiveBar.chatComposeActive)
#define g_HudUiMgrObjectiveChatComposeTextInput (g_HudUiMgr.objective.chatComposeTextInput)
#define g_HudUiMgrObjectiveCounterTextPanel (g_HudUiMgr.objective.counterTextPanel)
#define g_HudUiMgrSensorBlock (g_HudUiMgr.sensor)
#define g_HudUiMgrSensorTrackedProgressSlot (g_HudUiMgr.sensor.trackedProgressSlot)
#define g_HudUiMgrSensorPanel (g_HudUiMgr.sensor.sensorPanel)
#define g_HudUiMgrSensorOverlay (g_HudUiMgr.sensor.sensorOverlay)
#define g_HudUiMgrSensorMeter (g_HudUiMgr.sensor.sensorMeter)
#define g_HudUiMgrShieldMessageWidget (g_HudUiMgr.sensor.shieldMessageWidget)
#define g_HudUiMgrStringMenu (g_HudUiMgr.sensor.stringMenu)
#define g_HudUiMgrSensorTargetMarkerImages (g_HudUiMgr.sensor.targetMarkerImages)
#define g_HudUiMgrSensorTargetMarkerCount (g_HudUiMgr.sensor.targetMarkerCount)
#define g_HudUiMgrWeaponSlots (g_HudUiMgr.weaponSlots)
#define g_HudUiMgrReticleSnapRadiusSq (g_HudUiMgr.reticleSnapRadiusSq)
#define g_HudUiMgrWeaponState (g_HudUiMgr.weaponState)
#define g_HudUiMgrTimerPanel (g_HudUiMgr.timerPanel)
#define g_HudUiMgrTimerPanelFloat (g_HudUiMgr.timerPanelFloat)
#define g_HudUiMgrActiveModeCounterIndex (g_HudUiMgr.activeModeCounterIndex)
#define g_HudUiMgrModeCounters (g_HudUiMgr.modeCounters)
#define g_HudUiMgrActiveWeaponMessageIndex \
    (g_HudUiMgr.activeMessageSelection.activeWeaponMessageIndex)
#define g_HudUiMgrActiveWeaponSideIndex \
    (g_HudUiMgr.activeMessageSelection.activeWeaponSideIndex)
#define g_HudUiMgrMessages (g_HudUiMgr.messages)
#define g_HudUiMgrStatsList (g_HudUiMgr.statsList)
#define g_HudUiMgrStatsListState1 (g_HudUiMgr.statsListState1)
#define g_HudUiMgrStatsListState2 (g_HudUiMgr.statsListState2)
#define g_HudUiMgrStatsListState3 (g_HudUiMgr.statsListState3)
#define g_HudUiMgrStatsListState4 (g_HudUiMgr.statsListState4)
#define g_HudUiMgrStatsListState5 (g_HudUiMgr.statsListState5)
#define g_HudUiLoadingCheckpointRawProgress \
    (g_HudUiMgr.loadingCheckpointTable.checkpointProgressRaw)
#define g_HudUiLoadingCheckpointProgress \
    (g_HudUiMgr.loadingCheckpointTable.checkpointProgressNormalized)
#define g_HudUiLoadingCheckpointProgressScale (0.0186219737f)
#define g_HudUiLoadingCheckpointMaxIndex \
    (g_HudUiMgr.loadingCheckpointTable.maxCheckpointIndex)
#define g_HudUiLoadingCheckpointCurrentIndex \
    (g_HudUiMgr.loadingCheckpointTable.currentCheckpointIndex)
#define g_HudUiLoadingCheckpointCurrentProgress \
    (g_HudUiMgr.loadingCheckpointTable.currentProgress)
#define g_HudUiMgrTailBar (g_HudUiMgr.tailBar)

struct HudUiFlashPanel {
    static unsigned int __fastcall ComputeFlashBlendColor(
        unsigned int color0,
        unsigned int color1,
        float blend
    );
};

struct HudUiCompositePanelEntry : HudUiTransitionTextPanel {
    HudUiCompositePanelEntry * AssignCopy(const HudUiCompositePanelEntry *source);
    HudUiCompositePanelEntry * ConstructorCopy(
        const HudUiCompositePanelEntry *source
    );
    static HudUiCompositePanelEntry *__fastcall ConstructorCopyRange(
        const HudUiCompositePanelEntry *sourceBegin,
        const HudUiCompositePanelEntry *sourceEnd,
        HudUiCompositePanelEntry *destBegin
    );
};

/**
 * the natural VC5
 * std::vector<HudUiCompositePanelEntry> specialization destructor/clear body,
 * which destroys the concrete entries, frees the buffer, and resets the
 * begin/end/capacity cursor triple.
 * Purpose: record the source provenance of the template body selected by this
 * typedef and its uses; it is not a hand-authored wrapper function.
 */
/**
 * the natural VC5
 * std::vector<HudUiCompositePanelEntry> specialization count-insert body for
 * 0x2c0-byte entries, including its in-place and reallocation paths.
 * Purpose: record the source provenance of the template body selected by this
 * typedef and its uses; it is not a hand-authored wrapper function.
 */
typedef std::vector<HudUiCompositePanelEntry> HudUiCompositePanelVector;

struct HudUiCompositePanel : HudUiPanel {
    int activeEntryCount;
    HudUiCompositePanelVector entryVector;

    /**
     * Original inline constructor evidence: no standalone retail function
     * exists; observed in retail 0x4bb790 before the entry-count constructor
     * body installs the composite ftable and initializes entry history.
     * Purpose: initialize the base panel slice and empty composite state.
     */
    HudUiCompositePanel()
        : HudUiPanel(
            0,
            0,
            0
        ) {
        activeEntryCount = 0;
    }
    /**
     * Recovered original constructor form for retail 0x4bb790. BN caller
     * 0x403930 constructs messagesPanel with entry count 25 before the
     * locatorPanels array, which requires a member-initializer-capable
     * constructor instead of a default construction plus later body call.
     */
    HudUiCompositePanel(int entryCount);
    ~HudUiCompositePanel();
    virtual void SetPos(
        int x,
        int y
    );
    /**
     * Original inline helper; no standalone retail function exists. Retail
     * table slot 3 dispatches 0x4bb9f0 through the HudUiElement::SetPos slot,
     * while existing source call sites use the recovered layout-oriented name.
     * Purpose: keep local callers readable without adding a second dispatch slot.
     */
    void LayoutEntries(
        int x,
        int y
    ) {
        SetPos(
            x,
            y
        );
    }
    void ResizeEntryVectorAndRelayout(int entryCount);
    void ReapplyEntryCount();
    void ResizeEntryCount(
        int oldCount,
        int entryCount
    );
    void __cdecl SetTextFmt(
        const char *format,
        ...
    );
    void SetTextFmtV(
        const char *format,
        va_list args
    );
    virtual void ScrollHistory();
    void SetFont(
        const char *faceName,
        int height,
        int weight,
        int width,
        int italic,
        int charSet,
        int pitchAndFamily
    );
    void Update(float deltaSeconds);
};

struct HudFontStyle {
    unsigned int validMarker;
    const char *fontName;
    int fontSize;
    unsigned int textColor;
    unsigned int bkColor;
    unsigned int bkMode;
    unsigned int shadowEnabled;
    int fontWeight;
    unsigned int alignMode;

    HudFontStyle();
    ~HudFontStyle();
    void Destructor();
};

struct HudUiBackground : HudUiBackgroundContainer {
    HudUiBackgroundMemberCursorWidget cursorWidget;
    zVidImagePartial *primaryClipImage;
    zVidImagePartial *capturedCompositeImage;
    HudUiWidget backgroundImageWidgets[20];
    HudUiBackgroundVideoWidget backgroundVideoWidgets[10];
    HudUiBackgroundSoundEntry backgroundSounds[10];
    HudFontStyle fontStyles[20];
    HudUiTransitionTextPanel backgroundTextPanels[50];
    zReader::Node *loadedRoot;
    zReader::Node *cfgRoot;
    int uiOriginX;
    int uiOriginY;

    HudUiBackground();
    virtual ~HudUiBackground();
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
    zReader::Node * LoadZrdAndSection(
        zReader::Node *loadedRootNode,
        const char *sectionName,
        int capturePrimary
    );
    virtual void SetEnabled(int enabled);
    unsigned char __fastcall BindButtonsNodeToWidgetByName(
        zReader::Node *parentNode,
        HudUiWidget *widget,
        const char *name
    );
    int BindWidgetByName(
        zReader::Node *loadedSectionNode,
        HudUiWidget *widget,
        const char *name
    );
    int BindPrimitiveNodeToElement(
        zReader::Node *loadedSectionNode,
        HudUiElement *element,
        const char *name
    );
    void FreeLoadedTreeRoots(int unused);
};

struct HudCmdSimpleWidget : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this simple
     * command-widget construction as HudUiZrdWidget construction followed by
     * compiler-generated table emission.
     * Purpose: construct a simple command-dialog ZRD widget subobject.
     */
    HudCmdSimpleWidget() : HudUiZrdWidget() {}
    virtual void OnActivate();
};

struct HudCmdNextSetButton : HudUiZrdWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * next-set button construction as HudUiZrdWidget construction followed by
     * compiler-generated table emission.
     * Purpose: construct the next-set navigation subobject.
     */
    HudCmdNextSetButton() : HudUiZrdWidget() {}
    void OnActivate();
};

struct HudCmdPrevSetButton : HudUiZrdWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * previous-set button construction as HudUiZrdWidget construction followed
     * by compiler-generated table emission.
     * Purpose: construct the previous-set navigation subobject.
     */
    HudCmdPrevSetButton() : HudUiZrdWidget() {}
    void OnActivate();
};

struct HudCmdNextCommandButton : HudUiZrdWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * next-command button construction as HudUiZrdWidget construction followed
     * by compiler-generated table emission.
     * Purpose: construct the next-command navigation subobject.
     */
    HudCmdNextCommandButton() : HudUiZrdWidget() {}
    void OnActivate();
};

struct HudCmdPrevCommandButton : HudUiZrdWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * previous-command button construction as HudUiZrdWidget construction
     * followed by compiler-generated table emission.
     * Purpose: construct the previous-command navigation subobject.
     */
    HudCmdPrevCommandButton() : HudUiZrdWidget() {}
    void OnActivate();
};

struct HudCmdResetButton : HudUiZrdWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * reset button construction as HudUiZrdWidget construction followed by
     * compiler-generated table emission.
     * Purpose: construct the reset command-dialog subobject.
     */
    HudCmdResetButton() : HudUiZrdWidget() {}
    void OnActivate();
};

struct HudCmdSetListWidget : HudUiCycleSelectorWidget {

    /**
     * Original inline constructor evidence: BN 0x40a5b0 embeds this concrete
     * set-list construction as HudUiCycleSelectorWidget construction followed
     * by compiler-generated table emission.
     * Purpose: construct the command-group selector subobject.
     */
    HudCmdSetListWidget() : HudUiCycleSelectorWidget() {}
    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-cmd-dialog.type
 * @recoil-artifact emits .text recoil:function:0x40a920: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudCmdDialog type.
 */
struct HudCmdDialog : HudUiBackground {
    HudCmdSimpleWidget resumeButton;
    HudCmdResetButton resetButton;
    HudCmdCommandList commandList;
    HudCmdKeyAButton keyAButton;
    HudCmdKeyBButton keyBButton;
    HudCmdJoyButton joyButton;
    HudCmdMouseButton mouseButton;
    HudCmdSetListWidget setList;
    HudCmdNextSetButton nextSetButton;
    HudCmdPrevSetButton prevSetButton;
    HudCmdNextCommandButton nextCommandButton;
    HudCmdPrevCommandButton prevCommandButton;
    HudUiTransitionTextPanel promptPanel;
    HudUiPanel descriptionPanel;
    int captureState;

    HudCmdDialog();
    virtual ~HudCmdDialog();
    virtual void UpdateAll(float deltaTime);
    int SelectGroupRelative(int delta);
    int SelectCommandRelative(int delta);
    void RebuildCommandBindingListsForGroup(int groupIndex);
    void OnCommandSelectionChanged(int commandIndex);
    int ApplyPrimaryKeyRebind(
        int keyCode,
        int commandIndex
    );
    int ApplySecondaryKeyRebind(
        int keyCode,
        int commandIndex
    );
    int ApplyJoystickButtonRebind(
        int buttonCode,
        int commandIndex
    );
    int ApplyMouseButtonRebind(
        int buttonCode,
        int commandIndex
    );
};

struct HudOptionsDialog;

extern char g_HudUiOptionsPanel_ResolutionCycleNodeName[];
extern char g_HudUiOptionsPanel_MusicVolumeWidgetNodeName[];
extern char g_HudUiOptionsPanel_MusicEnableToggleNodeName[];
extern char g_HudUiOptionsPanel_SoundVolumeWidgetNodeName[];
extern char g_HudUiOptionsPanel_SoundQualitySelectorNodeName[];
extern char g_HudUiOptionsPanel_SoundActiveToggleNodeName[];
extern char g_EffectsZrdNodeName[8];
extern char g_HudUiOptionsPanel_TextureMemorySelectorNodeName[];
extern char g_HudUiOptionsPanel_ObjectDetailSelectorNodeName[];
extern char g_HudUiOptionsPanel_FullHudToggleNodeName[];
extern char g_HudUiOptionsPanel_PerspectiveToggleNodeName[];
extern char g_HudUiOptionsPanel_LightingToggleNodeName[];
extern char g_HudUiOptionsPanel_SectionName[];

struct HudUiOptionsPanelBackButton : HudUiZrdWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanelBackButton.
 */
HudUiOptionsPanelBackButton() {
    }

    void OnActivate();
};

struct HudUiOptionsPanel_Lighting : HudUiCheckToggleWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Lighting.
 */
HudUiOptionsPanel_Lighting() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_Perspective : HudUiCheckToggleWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Perspective.
 */
HudUiOptionsPanel_Perspective() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_FullHud : HudUiCheckToggleWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_FullHud.
 */
HudUiOptionsPanel_FullHud() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
};

struct HudUiOptionsPanel_ObjectDetail : HudUiCycleSelectorWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_ObjectDetail.
 */
HudUiOptionsPanel_ObjectDetail() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_TextureMemory : HudUiCycleSelectorWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_TextureMemory.
 */
HudUiOptionsPanel_TextureMemory() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_Effects : HudUiCycleSelectorWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Effects.
 */
HudUiOptionsPanel_Effects() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundActive : HudUiCheckToggleWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundActive.
 */
HudUiOptionsPanel_SoundActive() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundQuality : HudUiCycleSelectorWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundQuality.
 */
HudUiOptionsPanel_SoundQuality() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundVolume : HudUiFillBitmap {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundVolume.
 */
HudUiOptionsPanel_SoundVolume() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_MusicEnable : HudUiCheckToggleWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_MusicEnable.
 */
HudUiOptionsPanel_MusicEnable() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_MusicVolume : HudUiFillBitmap {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_MusicVolume.
 */
HudUiOptionsPanel_MusicVolume() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_Resolution : HudUiCycleSelectorWidget {
    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Resolution.
 */
HudUiOptionsPanel_Resolution() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-options-dialog.type
 * @recoil-artifact emits .text recoil:function:0x40cf00: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudOptionsDialog type.
 */
struct HudOptionsDialog : HudUiBackground {
    HudUiOptionsPanelBackButton backButton;
    HudUiOptionsPanel_Lighting lightingToggle;
    HudUiOptionsPanel_Perspective perspectiveToggle;
    HudUiOptionsPanel_FullHud fullHudToggle;
    HudUiOptionsPanel_ObjectDetail objectDetailSelector;
    HudUiOptionsPanel_TextureMemory textureMemorySelector;
    HudUiOptionsPanel_Effects effectsSelector;
    HudUiOptionsPanel_SoundActive soundActiveToggle;
    HudUiOptionsPanel_SoundQuality soundQualitySelector;
    HudUiOptionsPanel_SoundVolume soundVolumeWidget;
    HudUiOptionsPanel_MusicEnable musicEnableToggle;
    HudUiOptionsPanel_MusicVolume musicVolumeWidget;
    HudUiOptionsPanel_Resolution resolutionSelector;

    HudOptionsDialog();
    ~HudOptionsDialog();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-cmd-dialog-state.type
 * @recoil-artifact emits .text recoil:function:0x40bc70: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete HudCmdDialogState type.
 */
struct HudCmdDialogState : RecoilStateDialogHost {
    HudCmdDialogState();
    static void __cdecl StaticInitAndRegisterAtExit();
    static HudCmdDialogState *StaticInit();
    static void RegisterAtExit();
    static void __cdecl AtExitDestructor();
    static void QueueEnter();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~HudCmdDialogState();
};
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialogState) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialogState,
        m_dialog
    ) == 0x04
);

union HudCmdDialogStateStorage {
    unsigned long align;
    unsigned char bytes[sizeof(HudCmdDialogState)];
};
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialogStateStorage) == 0x08);

extern HudCmdDialogStateStorage g_HudCmdDialogState;
#define g_HudCmdDialogState \
    (*(HudCmdDialogState *)&g_HudCmdDialogState)

struct HudUiMessageBoxDialog : HudUiBackground {
    zVidRect32 blitRect;
    int modalResult;
    int modalFrameCountdown;
    int fallbackWidth;
    int fallbackHeight;
    zVidImagePartial *backgroundImage;
    zVidImagePartial *okButtonNormalImage;
    zVidImagePartial *okButtonPressedImage;
    HudUiWidget backdropWidget;
    HudUiPanel messagePanel;
    HudUiPanel titlePanel;
    HudUiMessageBoxOkButton okButton;
    HudUiMessageBoxCancelButton cancelButton;

    HudUiMessageBoxDialog * Constructor(
        const char *zrdPath,
        const char *sectionName
    );
    void Destructor();
    int RunModal(
        const char *messageText,
        const char *titleText,
        void *modalContext = 0,
        float timeoutSeconds = -1.0f
    );
    void OnOk();
    void OnCancel();
};

struct HudUiPanelLayoutEntry {
    HudUiPanel panel;
    int layoutX;
    int layoutY;

    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: retail 0x409570 directly constructs the embedded panel with
     * the three-argument HudUiPanel constructor before the nested EH lifetime.
     * Purpose: construct a temporary layout entry with a live embedded panel.
     */
    HudUiPanelLayoutEntry(
        const char *text,
        int x,
        int y
    ) : panel(
            text,
            x,
            y
        ) {
    }

#if !defined(_MSC_VER) || _MSC_VER >= 1200
    /*
     * Non-VC5 compatibility declarations for the bespoke container branch
     * below.  The VC5 retail path leaves these as implicit special members so
     * std::vector emits the original copy-construction and assignment bodies.
     */
    static HudUiPanelLayoutEntry *__fastcall CopyAssignRange(
        const HudUiPanelLayoutEntry *sourceStart,
        const HudUiPanelLayoutEntry *sourceEnd,
        HudUiPanelLayoutEntry *dest
    );
    static void __stdcall DestroyRange(
        HudUiPanelLayoutEntry *start,
        HudUiPanelLayoutEntry *end
    );
#endif
};

#if defined(_MSC_VER) && _MSC_VER < 1200

struct HudUiPanelSpan : public std::vector<HudUiPanelLayoutEntry> {
    void clear();
};
typedef std::vector<HudUiPanelSpan> HudUiPanelSpanVec;

#else

struct HudUiPanelSpan {
    unsigned int allocatorProxy;
    HudUiPanelLayoutEntry *first;
    HudUiPanelLayoutEntry *last;
    HudUiPanelLayoutEntry *limit;

    typedef HudUiPanelLayoutEntry *iterator;
    typedef const HudUiPanelLayoutEntry *const_iterator;

    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: retail 0x409570 establishes an outer automatic span lifetime
     * before entering the row and label loops.
     * Purpose: initialize an empty temporary panel-entry span.
     */
    HudUiPanelSpan()
        : allocatorProxy(0),
          first(0),
          last(0),
          limit(0) {
    }
    /**
     * Original-source helper; no standalone retail function exists.
     * Evidence: retail 0x409570 lowers its outer EH state before a direct
     * 0x2ac-stride panel-destruction loop and one operator delete call.
     * Purpose: destroy the temporary span entries and release their storage.
     */
    ~HudUiPanelSpan() {
        HudUiPanelLayoutEntry *entry = first;
        while (entry != last) {
            entry->panel.HudUiPanel::~HudUiPanel();
            ++entry;
        }
        ::operator delete(first);
    }
    iterator begin() {
        return first;
    }
    const_iterator begin() const {
        return first;
    }
    iterator end() {
        return last;
    }
    const_iterator end() const {
        return last;
    }
    void clear() {
        Clear();
    }
    void insert(
        iterator insertPos,
        unsigned int count,
        const HudUiPanelLayoutEntry &templatePanel
    ) {
        InsertN(
            insertPos,
            count,
            &templatePanel
        );
    }
    void insert(
        iterator insertPos,
        const HudUiPanelLayoutEntry &templatePanel
    ) {
        InsertN(
            insertPos,
            1,
            &templatePanel
        );
    }
    void Clear();
    HudUiPanelSpan * CopyInit(const HudUiPanelSpan *source);
    HudUiPanelSpan * CopyFrom(const HudUiPanelSpan *source);
    void InsertN(
        HudUiPanelLayoutEntry *insertPos,
        unsigned int count,
        const HudUiPanelLayoutEntry *templatePanel
    );
    void DestroyAndFree();
};

struct HudUiPanelSpanVec {
    unsigned char allocatorProxy;
    unsigned char allocatorPadding[3];
    HudUiPanelSpan *first;
    HudUiPanelSpan *last;
    HudUiPanelSpan *limit;

    typedef HudUiPanelSpan *iterator;
    typedef const HudUiPanelSpan *const_iterator;

    /**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4b92a0 HudUiListSelectorItem::HudUiListSelectorItem callers.
 * Purpose: preserve the recovered HUD behavior for HudUiPanelSpanVec.
 */
    HudUiPanelSpanVec() {
#if defined(_MSC_VER) && _MSC_VER < 1200
        char allocatorProxyValue;
#else
        char allocatorProxyValue = 0;
#endif
        allocatorProxy = allocatorProxyValue;
        first = 0;
        last = 0;
        limit = 0;
    }
    /**
     * Original-source helper; no standalone retail function is required.
     * Evidence: the complete destructors at 0x4091e0 and 0x4092a0 tear down the
     * embedded row-span vector before their enclosing widget/base lifetimes.
     * Purpose: destroy every owned row span, release vector storage, and reset
     * the vector range.
     */
    ~HudUiPanelSpanVec() {
        HudUiPanelSpan *row = first;
        while (row != last) {
            row->DestroyAndFree();
            ++row;
        }

        ::operator delete(first);
        first = 0;
        last = 0;
        limit = 0;
    }
    iterator begin() {
        return first;
    }
    const_iterator begin() const {
        return first;
    }
    iterator end() {
        return last;
    }
    const_iterator end() const {
        return last;
    }
    void insert(
        iterator insertPos,
        unsigned int count,
        const HudUiPanelSpan &templateSpan
    ) {
        InsertN(
            insertPos,
            count,
            &templateSpan
        );
    }
    void insert(
        iterator insertPos,
        const HudUiPanelSpan &templateSpan
    ) {
        InsertN(
            insertPos,
            1,
            &templateSpan
        );
    }
    void InsertN(
        HudUiPanelSpan *insertPos,
        unsigned int count,
        const HudUiPanelSpan *templateSpan
    );
};

#endif

struct HudUiCreditsBackButton : HudUiZrdWidget {
    virtual void OnActivate();
};

struct HudUiCreditsQuitButton : HudUiZrdWidget {
    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-zrd-scrolling-text.type
 * @recoil-artifact emits .text recoil:function:0x409360: VC5 scalar deleting destructor for this virtual-destructor model.
 */
struct HudUiZrdScrollingText : HudUiZrdWidget {
    HudUiPanelSpanVec rows;
    HudUiRect rect;
    int totalHeight;

    HudUiZrdScrollingText();
    /**
     * Purpose: declare the retained ordinary destructor defined inline at its
     * source-order position in the owning translation unit.
     */
    ~HudUiZrdScrollingText();
    void OnActivate();
    void OnActivateResetOwnerFade();
    void Update(float deltaSeconds);
    void UpdateScrollPositions(float scrollProgress);
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-credits-panel.type
 * @recoil-artifact emits .text recoil:function:0x4091c0: VC5 scalar deleting destructor for this virtual-destructor model.
 */
struct HudUiCreditsPanel : HudUiBackground {
    float fadeStep;
    HudUiCreditsBackButton backButton;
    HudUiCreditsQuitButton quitButton;
    HudUiZrdScrollingText creditsScreen;
    float fadeProgress;

    HudUiCreditsPanel();
    /**
     * Purpose: let ordinary C++ lifetime rules tear down the credits widgets
     * and background base in reverse construction order.
    */
    ~HudUiCreditsPanel();
    virtual void UpdateAll(float deltaSeconds);
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanelsimple-constructordefaultthunk
 * @recoil-artifact emits .text recoil:function:0x40fab0: VC5 default-constructor closure used for array construction.
 * Purpose: let VC5 supply the no-argument array element callback from the
 * parameterized constructor's default arguments.
 */
struct HudUiPanelSimple : HudUiPanel {
    HudUiPanelSimple(
        const char *text = 0,
        int x = 0,
        int y = 0
    );
};

struct HudUiShieldMessageWidget {
    int state;
    int viewportResetFrame;
    unsigned char unknown_08[0x04];
    HudUiRect screenRect;
    HudUiWidget widget;
    HudUiPanelSimple percentTextPanel;
    HudUiShieldMeterCandidate meter;
    unsigned char unknown_4bc[0x08];

    static int __stdcall ApplyLayout(zReader::Node *layoutRoot);
};

typedef HudUiShieldMessageWidget HudUiShieldMessageWidgetState;

/**
 * HudUiTimerPanelFloat owner evidence: BN constructor 0x40ef60 installs the
 * class table at 0x4ce7d8 after HudUiPanel construction, and BN table slot 1
 * references Draw at 0x40f040 as the only derived override found for this
 * class.
 */
struct HudUiTimerPanelFloat : HudUiPanel {
    float sampleElapsedSec;
    float displayValue;
    float sampleFrameCount;

    HudUiTimerPanelFloat();
    void Draw();
};

/**
 * HudUiStringMenu owner evidence: BN InitHudLayouts 0x40f4c0 allocates
 * 0x3cdc bytes, constructs the HudUiContainer base, array-constructs 23
 * HudUiPanelSimple items at offset 0x20, installs the string-menu class
 * table, and later ShutdownResources 0x40fbd0 destroys the complete object.
 */
struct HudUiStringMenu : HudUiContainer {
    unsigned char unknown_10[0x10];
    HudUiPanelSimple items[23];

    HudUiStringMenu();
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zhud.hud-ui-stats-list-element.type
 * @recoil-artifact emits .text recoil:function:0x40fa20: VC5 scalar deleting destructor emitted for this virtual-destructor model.
 * HudUiStatsListElement owner evidence: BN InitHudLayouts 0x40f4c0 installs
 * the derived table while constructing this element, with scalar deleting
 * destructor slot zero and Update 0x40fa10 at slot +0x24.
 * Purpose: Record compiler-generated lifecycle code emitted by the complete stats-list element type.
 */
struct HudUiStatsListElement : HudUiElement {
    /**
     * Original-source helper; no standalone retail constructor exists.
     * Evidence: InitHudLayouts 0x40f4c0 inlines the HudUiElement base
     * construction, installs the stats-list table, and clears triplet.
     * Purpose: establish the stats-list dynamic type before its triplet is allocated.
     */
    HudUiStatsListElement() : HudUiElement(0, 0), triplet(0) {}
    virtual ~HudUiStatsListElement();
    void Update(float deltaSeconds);

    HudUiTriplet *triplet;
};

struct HudUiTimerPanel : HudUiPanel {
    float elapsedSeconds;
    int stopped;
    int secondsStep;

    HudUiTimerPanel();
    static void __fastcall SetRunning(int running);
    static void __stdcall SetElapsedSeconds(float seconds);
    static void __stdcall SetSeconds(
        float elapsedSeconds,
        float secondsStep
    );
    static float GetSeconds();
    static void __fastcall ZarWriteTimerDataCallback(
        zZbdSectionCallbackCtx *sectionCtx,
        HudUiTimerPanel *userData
    );
    static void __stdcall ZarReadTimerData(
        const float *buffer,
        int byteCount,
        HudUiTimerPanel *userData
    );
    void Update(float deltaSeconds);
    void UpdateHMSFromSeconds(float seconds);
    void SetTimeSeconds(
        int hours,
        int minutes,
        int seconds
    );
};

struct HudUiCounterTextPanel : HudUiPanel {
    HudUiCounterTextPanel();
};

struct HudUiScoreboardEntry {
    int playerKey;
    char displayName[0x40];
    int score;
    int lapCount;
    unsigned int playerColorPackedRgb;
};

struct HudUiTripletEntries {
    unsigned char rowInitFlag;
    unsigned char padding[3];
    HudUiScoreboardEntry *begin;
    HudUiScoreboardEntry *end;
    HudUiScoreboardEntry *cap;

    int GetCount();
    static HudUiScoreboardEntry *__stdcall CopyRange(
        HudUiScoreboardEntry *sourceBegin,
        HudUiScoreboardEntry *sourceEnd,
        HudUiScoreboardEntry *dest
    );
    static void __stdcall FillN(
        HudUiScoreboardEntry *dest,
        unsigned int count,
        const HudUiScoreboardEntry *sourceValue
    );
    void insert(
        HudUiScoreboardEntry *insertPos,
        unsigned int count,
        const HudUiScoreboardEntry &sourceValue
    ) {
        if ((unsigned int)(cap - end) < count) {
            const unsigned int currentCount =
                begin != 0 ? (unsigned int)(end - begin) : 0;
            const unsigned int newCapacity =
                currentCount + (count < currentCount ? currentCount : count);
            HudUiScoreboardEntry *const newBegin =
                (HudUiScoreboardEntry *)(::operator new(
                    newCapacity * sizeof(HudUiScoreboardEntry)
                ));
            FillN(newBegin, count, &sourceValue);
            CopyRange(insertPos, end, newBegin + count);
            ((StdPtrVector *)(this))->ClearNoOpDestroy(
                (int *)(begin),
                (int *)(end)
            );
            ::operator delete(begin);
            cap = newBegin + newCapacity;
            end = newBegin + GetCount() + count;
            begin = newBegin;
            return;
        }

        if ((unsigned int)(end - insertPos) < count) {
            CopyRange(insertPos, end, insertPos + count);
            FillN(
                end,
                count - (unsigned int)(end - insertPos),
                &sourceValue
            );
            std::fill(insertPos, end, sourceValue);
            end += count;
        } else if (count > 0) {
            CopyRange(end - count, end, end);
            std::copy_backward(insertPos, end - count, end);
            std::fill(insertPos, insertPos + count, sourceValue);
            end += count;
        }
    }
    HudUiScoreboardEntry *erase(
        HudUiScoreboardEntry *erasePos
    ) {
        std::copy(erasePos + 1, end, erasePos);
        --end;
        return erasePos;
    }
};

struct HudUiTriplet : HudUiContainer {
    HudUiPanel *headerPanels[3];
    HudUiPanel *rowCells[24];
    HudUiTripletEntries entries;
    int baseX;
    int baseY;
    int rowPitchY;
    int lapsColumnOffsetX;
    int killsColumnOffsetX;
    int fontSize;
    int fontWeight;
    int baseXStart;
    int baseYStart;
    int baseXEnd;
    int baseYEnd;
    int rowPitchYStart;
    int rowPitchYEnd;
    int lapsColumnOffsetXStart;
    int lapsColumnOffsetXEnd;
    int killsColumnOffsetXStart;
    int killsColumnOffsetXEnd;
    int fontSizeStart;
    int fontSizeEnd;
    int fontWeightStart;
    int fontWeightEnd;

    static void __cdecl StaticInitWndClassNameAndRegisterAtExit();
    static CString *ConstructWndClassName();
    static void RegisterWndClassNameDtorAtExit();
    static void __cdecl DestroyWndClassName();
    HudUiTriplet();
    ~HudUiTriplet();
    void InterpolateLayout(float t);
    void RebuildDisplay();
    void AddEntry(GameNetPlayerRow *entryData);
    void UpdateEntryData(GameNetPlayerRow *entryData);
    void RemoveEntry(GameNetPlayerRow *entryKey);
    int IsLocalPlayerFirstEntry();
};

/**
 * BN constructors at 0x4bd020 and 0x4bd2d0 initialize four HudUiPanel rows
 * immediately after the HudUiContainer base; row stride is sizeof(HudUiPanel).
 */
struct HudUiTextStack4 : HudUiContainer {
    HudUiPanel lines[4];

    HudUiPanel * PushLine(
        const char *message,
        float duration
    );
    void Clear();
    void SetFontAll(
        const char *faceName,
        int height,
        int weight,
        int width
    );
    void SetTextColors(
        unsigned int color0,
        unsigned int color1
    );
    void SetXAll(int x);
    void SetYDescending(int yStart);
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitopmessagestack-destructorcore
 * @recoil-artifact emits .text recoil:function:0x40fe90: VC5-generated implicit cleanup for the top-message stack.
 * Purpose: Record the compiler-generated destruction of the top-message rows and container base.
 */
struct HudUiTopMessageStack : HudUiTextStack4 {
    HudUiTopMessageStack * Constructor();
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduichatmessagestack-destructorcore
 * @recoil-artifact emits .text recoil:function:0x40fef0: VC5-generated implicit cleanup for the chat-message stack.
 * Purpose: Record the compiler-generated destruction of the chat-message rows and container base.
 */
struct HudUiChatMessageStack : HudUiTextStack4 {
    HudUiChatMessageStack * Constructor();
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(HudUiRect) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackNode,
        next
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackNode,
        trackKind
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackNode,
        payload
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrSensorTrackList) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackList,
        trackListAux
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackList,
        head
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackList,
        tail
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorTrackList,
        count
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiElement) == 0x34);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        next
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        parent
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        flags
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        timer
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        x
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        y
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        bltSource
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        clipRect
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiElement,
        state
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCircle) == 0x40);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCircle,
        radius
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCircle,
        radiusSquared
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCircle,
        color565
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutBase,
        layoutRect
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutBase,
        activeRect
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutBase,
        widget0
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(HudLayoutBase) == 0xec);
RECOIL_STATIC_ASSERT(sizeof(HudLayoutSW) == 0xec);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget0
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget1
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget1ImageDefault
    ) == 0x1a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget1Image320
    ) == 0x1ac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget1Image400
    ) == 0x1b0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget2
    ) == 0x1b4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget2ImageDefault
    ) == 0x270
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget2Image320
    ) == 0x274
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget2Image400
    ) == 0x278
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        widget3
    ) == 0x27c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        reticleClipRect
    ) == 0x338
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLayoutHW,
        reticleClipInitFlags
    ) == 0x348
);
RECOIL_STATIC_ASSERT(sizeof(HudLayoutHW) == 0x34c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorRectScaled
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorRectRaw
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorPiVSrcRect
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorClampHalfW
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorClampHalfH
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorParam
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorRangeSq
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        trackedProgressSlot
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorPanel
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorOverlay
    ) == 0x114
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        sensorMeter
    ) == 0x1d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        shieldMessageWidget
    ) == 0x310
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        stringMenu
    ) == 0x314
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        targetMarkerImages
    ) == 0x318
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrSensorBlock,
        targetMarkerCount
    ) == 0x32c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiShieldMessageWidget) == 0x4c4);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiShieldMessageWidget,
        widget
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiShieldMessageWidget,
        percentTextPanel
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiShieldMessageWidget,
        meter
    ) == 0x37c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiContainer) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiContainer,
        enabled
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiContainer,
        childHead
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiContainer,
        childTail
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiRectDirty) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        ownsImage
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        dirtyRectCount
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        image
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        imageStateWord
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPrimitiveBindTarget,
        endX
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPrimitiveBindTarget,
        endY
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPrimitiveBindTarget,
        color565
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        bltClipRectOrNull
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        alignFlags
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiWidget,
        dirtyRects
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiWidget) == 0xbc);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundCursorWidget,
        capturedImage
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundCursorWidget,
        captureEnabled
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundCursorWidget,
        captureSourceSelector
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundCursorWidget) == 0xd0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundVideoWidget,
        stream
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundVideoWidget,
        elapsedTimeSec
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundVideoWidget,
        colorKey565
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundVideoWidget,
        mediaPath
    ) == 0x3e
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundVideoWidget) == 0x144);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextLabel) == 0x148);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        textBuffer
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        fontHandle
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        centerText
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        centerBoundsLeft
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        centerBoundsRight
    ) == 0x140
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextLabel,
        alignMode
    ) == 0x144
);
#if defined(_MSC_VER) && _MSC_VER < 1200
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelPtrVector) == 0x10);
#endif
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidget) == 0x14c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        originX
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        originY
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        modeOrEnabled
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        owner
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        boundsRect
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        defaultImage
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        disabledImage
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        rolloverImage
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        rolloverSound
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        rolloverPlayHandle
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        rolloverSoundScale
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        activateImage
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        activateSound
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        activatePlayHandle
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        activateSoundScale
    ) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        disabledSound
    ) == 0x104
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        disabledSoundScale
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        labelPanels
    ) == 0x10c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        rolloverLabelPanels
    ) == 0x11c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        activateLabelPanels
    ) == 0x12c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidget,
        disabledLabelPanels
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCheckToggleWidget) == 0x164);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        checked
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        disabledCheckedImage
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        disabledCheckedFallbackImage
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        uncheckedImage
    ) == 0x158
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        checkedImage
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCheckToggleWidget,
        checkedLabelPanel
    ) == 0x160
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCycleSelectorWidget) == 0x208);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        selectedIndex
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        itemCount
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        firstIndex
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        visibleCount
    ) == 0x158
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        fontStyleRef
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        textOffsetX
    ) == 0x160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        textOffsetY
    ) == 0x164
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        entriesA
    ) == 0x168
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCycleSelectorWidget,
        entriesB
    ) == 0x1b8
);
RECOIL_STATIC_ASSERT(sizeof(HudUiFillBitmap) == 0x188);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        normalizedValue
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        fillImage
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        fillRect
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        fillOffsetX
    ) == 0x164
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        fillOffsetY
    ) == 0x168
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        previewImage
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        previewRect
    ) == 0x170
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        previewOffsetX
    ) == 0x180
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiFillBitmap,
        previewOffsetY
    ) == 0x184
);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidgetEx17C_Item) == 0x17c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        selected
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        ownerSelector
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        itemIndex
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        mouseRectValid
    ) == 0x158
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        mouseRect
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        selectedImage
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        unselectedImage
    ) == 0x170
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        selectedRolloverImage
    ) == 0x174
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C_Item,
        unselectedRolloverImage
    ) == 0x178
);
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdWidgetEx17C) == 0x17c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C,
        optionCount
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C,
        options
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdWidgetEx17C,
        selectedIndex
    ) == 0x178
);
RECOIL_STATIC_ASSERT(sizeof(HudUiListSelectorItem) == 0x2ac);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiListSelectorItem,
        entryIndex
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiListSelectorItem,
        owner
    ) == 0x2a8
);
#if !defined(_MSC_VER) || _MSC_VER >= 1200
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindingVector) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        first
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        last
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        limit
    ) == 0x0c
);
#endif
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingEntry,
        displayText
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingEntry,
        commandId
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindingEntry) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindButtonBase) == 0x44c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        bindingSlotTotalCount
    ) == 0x164
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        visibleBindingSlotCount
    ) == 0x168
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        bindPanel
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        bindingSlotPanels
    ) == 0x418
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        bindingVec
    ) == 0x41c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        bindingSlotSpacing
    ) == 0x42c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        selectedBindingIndex
    ) == 0x430
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        visibleListOffsetX
    ) == 0x434
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        visibleListOffsetY
    ) == 0x438
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        overflowListOffsetX
    ) == 0x43c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        overflowListOffsetY
    ) == 0x440
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        selectedFontStyleRef
    ) == 0x444
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindButtonBase,
        listFontStyleRef
    ) == 0x448
);
RECOIL_STATIC_ASSERT(sizeof(HudCmdCommandList) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdKeyAButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdKeyBButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdJoyButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdMouseButton) == 0x44c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdSimpleWidget) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdNextSetButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdPrevSetButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdNextCommandButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdPrevCommandButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudCmdSetListWidget) == 0x208);
RECOIL_STATIC_ASSERT(sizeof(HudCmdDialog) == 0xce00);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        resumeButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        resetButton
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        commandList
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        keyAButton
    ) == 0xb030
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        keyBButton
    ) == 0xb47c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        joyButton
    ) == 0xb8c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        mouseButton
    ) == 0xbd14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        setList
    ) == 0xc160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        nextSetButton
    ) == 0xc368
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        prevSetButton
    ) == 0xc4b4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        nextCommandButton
    ) == 0xc600
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        prevCommandButton
    ) == 0xc74c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        promptPanel
    ) == 0xc898
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        descriptionPanel
    ) == 0xcb58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdDialog,
        captureState
    ) == 0xcdfc
);
RECOIL_STATIC_ASSERT(sizeof(HudOptionsDialog) == 0xbec4);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        backButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        lightingToggle
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        perspectiveToggle
    ) == 0xabfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        fullHudToggle
    ) == 0xad60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        objectDetailSelector
    ) == 0xaec4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        textureMemorySelector
    ) == 0xb0cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        effectsSelector
    ) == 0xb2d4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        soundActiveToggle
    ) == 0xb4dc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        soundQualitySelector
    ) == 0xb640
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        soundVolumeWidget
    ) == 0xb848
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        musicEnableToggle
    ) == 0xb9d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        musicVolumeWidget
    ) == 0xbb34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudOptionsDialog,
        resolutionSelector
    ) == 0xbcbc
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundContainer) == 0x44);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundContainer,
        inputFocusElement
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundContainer,
        captureTransitionMask
    ) == 0x40
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackgroundSoundEntry) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundSoundEntry,
        volume
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackgroundSoundEntry,
        playHandle
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiDialogController,
        capturedImage
    ) == 0x114
);
RECOIL_STATIC_ASSERT(sizeof(HudUiDialogController) == 0x118);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        cursorWidget
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        primaryClipImage
    ) == 0x114
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        capturedCompositeImage
    ) == 0x118
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        backgroundImageWidgets
    ) == 0x11c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        backgroundVideoWidgets
    ) == 0xfcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        backgroundSounds
    ) == 0x1c74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        fontStyles
    ) == 0x1cec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        backgroundTextPanels
    ) == 0x1fbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        loadedRoot
    ) == 0xa93c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        cfgRoot
    ) == 0xa940
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        uiOriginX
    ) == 0xa944
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBackground,
        uiOriginY
    ) == 0xa948
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBackground) == 0xa94c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        blitRect
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        modalResult
    ) == 0xa95c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        modalFrameCountdown
    ) == 0xa960
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        fallbackWidth
    ) == 0xa964
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        fallbackHeight
    ) == 0xa968
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        backgroundImage
    ) == 0xa96c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        okButtonNormalImage
    ) == 0xa970
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        okButtonPressedImage
    ) == 0xa974
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        backdropWidget
    ) == 0xa978
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        messagePanel
    ) == 0xaa34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        titlePanel
    ) == 0xacd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        okButton
    ) == 0xaf7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessageBoxDialog,
        cancelButton
    ) == 0xb0c8
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxDialog) == 0xb214);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelLayoutEntry) == 0x2ac);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelLayoutEntry,
        layoutX
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelLayoutEntry,
        layoutY
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSpan) == 0x10);
#if !defined(_MSC_VER) || _MSC_VER >= 1200
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        first
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        last
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        limit
    ) == 0x0c
);
#endif
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSpanVec) == 0x10);
#if !defined(_MSC_VER) || _MSC_VER >= 1200
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        first
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        last
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        limit
    ) == 0x0c
);
#endif
RECOIL_STATIC_ASSERT(sizeof(HudUiZrdScrollingText) == 0x170);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdScrollingText,
        rows
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdScrollingText,
        rect
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiZrdScrollingText,
        totalHeight
    ) == 0x16c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCreditsPanel,
        fadeStep
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCreditsPanel,
        backButton
    ) == 0xa950
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCreditsPanel,
        quitButton
    ) == 0xaa9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCreditsPanel,
        creditsScreen
    ) == 0xabe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCreditsPanel,
        fadeProgress
    ) == 0xad58
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCreditsPanel) == 0xad5c);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxOkButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessageBoxCancelButton) == 0x14c);
RECOIL_STATIC_ASSERT(sizeof(HudUiTransitionTextPanel) == 0x2c0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashCountdown
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashResetValue
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashAltColor0
    ) == 0x2ac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashAltColor1
    ) == 0x2b0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashEnabled
    ) == 0x2b4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashMode
    ) == 0x2b8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTransitionTextPanel,
        flashDirectionSign
    ) == 0x2bc
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCompositePanelEntry) == 0x2c0);
RECOIL_STATIC_ASSERT(sizeof(HudUiCompositePanelVector) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCompositePanel,
        activeEntryCount
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCompositePanel,
        entryVector
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(sizeof(HudFontStyle) == 0x24);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        validMarker
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        fontName
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        fontSize
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        textColor
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        bkColor
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        bkMode
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        shadowEnabled
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        fontWeight
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudFontStyle,
        alignMode
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(HudUiTripletPanel) == 0x270);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTripletPanel,
        visibleCount
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTripletPanel,
        items
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiNanitePanel) == sizeof(HudUiTripletPanel));
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessage,
        variantImages
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessage,
        activeSideImages
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessage,
        sideImageSwaps
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessage,
        panel
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelFull,
        layoutX
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelFull,
        layoutY
    ) == 0x2ac
);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelFontParams) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMessage,
        widget
    ) == 0x390
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMessage) == 0x44c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCounter,
        stateImages
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCounter,
        clipViewportRect
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCounter,
        layoutX
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCounter,
        layoutY
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCounter) == 0xe0);
RECOIL_STATIC_ASSERT(sizeof(HudUiBarPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudUiPolylinePoint) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudUiPolyline) == 0xe8);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPolyline,
        points
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPolyline,
        pointCount
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPolyline,
        color565
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPolyline,
        clipRect
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(sizeof(HudUiSliderBorder) == 0x114);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        originX
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        originY
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        halfWidth
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        height
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        blinkEnabled
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        blinkPeriodSec
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        blinkTimeRemainingSec
    ) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        blinkDirSign
    ) == 0x104
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        caretHalfWidth
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        inputActive
    ) == 0x10c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        sliderVisibleWhenInputActive
    ) == 0x110
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSliderBorder,
        rawKeyFilterEnabled
    ) == 0x111
);
RECOIL_STATIC_ASSERT(sizeof(HudUiBar) == 0x140);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBar,
        points
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBar,
        drawVertexCount
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBar,
        drawParam
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBar,
        quadHeight
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiBar,
        quadLeftX
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiManagerMeterBaseCandidate) == 0x140);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiManagerMeterBaseCandidate,
        points
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiManagerMeterBaseCandidate,
        drawVertexCount
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiManagerMeterBaseCandidate,
        color565
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiManagerMeterBaseCandidate,
        fillPixelsMax
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiManagerMeterBaseCandidate,
        meterFlags
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiManagerMeterCandidate) == 0x140);
RECOIL_STATIC_ASSERT(sizeof(HudUiShieldMeterCandidate) == 0x140);
RECOIL_STATIC_ASSERT(sizeof(HudUiObjectiveBar) == 0x140);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiObjectiveBar,
        points
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiObjectiveBar,
        drawVertexCount
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiObjectiveBar,
        drawParam
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiObjectiveBar,
        slideRangeX
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiObjectiveBar,
        chatComposeActive
    ) == 0x13c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextInput) == 0x110);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextInput,
        buffer
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextInput,
        capacity
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextInput,
        cursor
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextInput,
        keyActionMap
    ) == 0x10
);
RECOIL_STATIC_ASSERT(sizeof(HudUiOwnedTextInput) == 0x114);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiOwnedTextInput,
        owner
    ) == 0x110
);
RECOIL_STATIC_ASSERT(sizeof(HudUiNumericTextInput) == 0x374);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNumericTextInput,
        textInput
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(HudUiNumericTextInput, textInput) +
    offsetof(HudUiOwnedTextInput, owner) == 0x25c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNumericTextInput,
        sliderBorder
    ) == 0x260
);
RECOIL_STATIC_ASSERT(sizeof(HudUiClampedIntTextInput) == 0x37c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiClampedIntTextInput,
        minValue
    ) == 0x374
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiClampedIntTextInput,
        maxValue
    ) == 0x378
);
RECOIL_STATIC_ASSERT(sizeof(HudUiClampedIntStepButton) == 0x154);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiClampedIntStepButton,
        targetInput
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiClampedIntStepButton,
        stepDelta
    ) == 0x150
);
RECOIL_STATIC_ASSERT(sizeof(HudUiSlot) == 0x1c0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        screenEdgeCode
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        trackNode
    ) == 0x38
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        screenX
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        screenY
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        slotWidget
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiSlot,
        trackMarkerWidget
    ) == 0x104
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrObjectiveBlock) == 0x53c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        objectiveWidget
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        objectiveSensorRect
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        objectiveMeter
    ) == 0x19c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        objectiveBar
    ) == 0x2e8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        chatComposeTextInput
    ) == 0x428
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrObjectiveBlock,
        counterTextPanel
    ) == 0x538
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        currentLayout
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        hudRootPanel
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        reticleWidget
    ) == 0x364
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        nanitePanel
    ) == 0x420
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        objective
    ) == 0x690
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        sensor
    ) == 0xbcc
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrSensorBlock) == 0x330);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        weaponSlots
    ) == 0xefc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        reticleSnapRadiusSq
    ) == 0x46fc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        weaponState
    ) == 0x4700
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        unknown_4704
    ) == 0x4704
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        timerPanel
    ) == 0x4784
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        timerPanelFloat
    ) == 0x4788
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        activeModeCounterIndex
    ) == 0x478c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        modeCounters
    ) == 0x4790
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        activeMessageSelection
    ) == 0x4b10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        messages
    ) == 0x4b18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        statsList
    ) == 0x7610
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        statsListState5
    ) == 0x7624
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLoadingCheckpointTable,
        checkpointProgressRaw
    ) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLoadingCheckpointTable,
        checkpointProgressNormalized
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLoadingCheckpointTable,
        maxCheckpointIndex
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLoadingCheckpointTable,
        currentCheckpointIndex
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudLoadingCheckpointTable,
        currentProgress
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(sizeof(HudLoadingCheckpointTable) == 0xdc);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        loadingCheckpointTable
    ) == 0x7628
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMgrData,
        tailBar
    ) == 0x7704
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrMessageSelectionState) == 0x8);
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrData) == 0x7844);

union HudUiMgrDataStorage {
    unsigned long align;
    unsigned char bytes[sizeof(HudUiMgrData)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMgrDataStorage) == 0x7844);

#define g_HudUiMgr \
    (*(HudUiMgrData *)&g_HudUiMgr)

RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textPick
    ) == 0x148
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textColor0
    ) == 0x14c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textColor1
    ) == 0x150
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        hFont
    ) == 0x154
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        cachedText
    ) == 0x15c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textWidthPx
    ) == 0x25c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textHeightPx
    ) == 0x260
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        shadowEnabled
    ) == 0x264
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        bkMode
    ) == 0x268
);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanel) == 0x2a4);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textDirty
    ) == 0x270
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        wordWrapEnabled
    ) == 0x278
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        wrapRect
    ) == 0x27c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        textRect
    ) == 0x28c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        shadowOffsetX
    ) == 0x29c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanel,
        shadowOffsetY
    ) == 0x2a0
);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSimple) == 0x2a4);
RECOIL_STATIC_ASSERT(sizeof(HudUiTimerPanelFloat) == 0x2b0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanelFloat,
        sampleElapsedSec
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanelFloat,
        displayValue
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanelFloat,
        sampleFrameCount
    ) == 0x2ac
);
RECOIL_STATIC_ASSERT(sizeof(HudUiStringMenu) == 0x3cdc);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiStringMenu,
        items
    ) == 0x20
);
RECOIL_STATIC_ASSERT(sizeof(HudUiStatsListElement) == 0x38);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiStatsListElement,
        triplet
    ) == 0x34
);
RECOIL_STATIC_ASSERT(sizeof(HudUiTimerPanel) == 0x2b0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanel,
        elapsedSeconds
    ) == 0x2a4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanel,
        stopped
    ) == 0x2a8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTimerPanel,
        secondsStep
    ) == 0x2ac
);
RECOIL_STATIC_ASSERT(sizeof(HudUiCounterTextPanel) == 0x2a4);
RECOIL_STATIC_ASSERT(sizeof(HudUiTripletEntries) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(HudUiScoreboardEntry) == 0x50);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiScoreboardEntry,
        displayName
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiScoreboardEntry,
        score
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiScoreboardEntry,
        lapCount
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiScoreboardEntry,
        playerColorPackedRgb
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiTriplet) == 0xe0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        headerPanels
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        rowCells
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        entries
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        lapsColumnOffsetX
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        killsColumnOffsetX
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        fontSize
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTriplet,
        fontWeight
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(sizeof(HudUiTextStack4) == 0xaa0);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiTextStack4,
        lines
    ) == 0x10
);
#endif
