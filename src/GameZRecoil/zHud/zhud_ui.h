#pragma once

#include "recoil/recoil_types.h"
#include <cstdarg>
#include <stddef.h>

#include <windows.h>

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zInput/zInput.h"
#include "recoil/recoil_callconv.h"
#include "zClipAlt.h"

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
struct HudUiMeter;
struct GameNetPlayerRow;
struct zZbdSectionCallbackCtx;
struct zSndSample;
struct zSndPlayHandle;
struct zFMV_Stream;

extern int g_HudCmdMouseDebounceFrames;
struct HudUiWidget;
struct HudUiMgrSensorTrackNode;
struct PlayerProgressTargetSlotRuntime;
struct HudUiNetGameSetupOverlayOwner;
extern zVidImagePartial *g_HudUiWidget_ExclusiveDrawImage;
extern HudUiContainer g_HudUiMgr;
extern HudUiTransitionTextPanel g_HudUiMgrHudRootPanel;
extern HudUiTimerPanel *g_HudUiMgrTimerPanel;
extern HudUiTimerPanelFloat *g_HudUiMgrTimerPanelFloat;
extern HudUiStringMenu *g_HudUiMgrStringMenu;
extern HudUiStatsListElement *g_HudUiMgrStatsList;
extern zVidImagePartial *g_HudUiMgrReticleImages[3];
extern zVidImagePartial *g_HudUiMgrSensorTargetMarkerImages[5];
extern HudUiNanitePanel g_HudUiMgrNanitePanel;
extern HudUiMessage g_HudUiMgrMessages[10];
extern int g_HudUiMgrActiveWeaponMessageIndex;
extern int g_HudUiMgrActiveWeaponSideIndex;
extern HudUiCounter g_HudUiMgrModeCounters[4];
extern HudUiSlot g_HudUiMgrWeaponSlots[32];
extern HudUiSlot *g_HudUiMgrSensorTrackedProgressSlot;
extern int g_HudUiMgrSensorRoundRobinTrackIndex;
extern HudUiMeter g_HudUiMgrObjectiveMeter;
extern int g_HudUiMgrActiveModeCounterIndex;
extern int g_HudUiMgrHudLayoutsInitialized;
extern int g_HudUiMgrHudLoaded;
extern int g_HudUiMgrLayoutDelayFrames;
extern int g_HudUiMgrSensorTargetMarkerCount;
extern int g_HudUiMgrWeaponState;
extern int g_HudUiMgrHudOriginX;
extern int g_HudUiMgrHudOriginY;
extern HudUiNetGameSetupOverlayOwner g_HudUiNetGameSetupOverlayOwner;

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

    HudUiElement() {
    }
    HudUiElement(
        int x,
        int y
    );
    ~HudUiElement() {
    }
    HudUiElement * Constructor(
        int x,
        int y
    );
    HudUiElement * CopyConstructor(const HudUiElement *source);
    HudUiElement * CopyFrom(const HudUiElement *source);
    virtual HudUiElement * ScalarDeletingDestructor(unsigned int flags);
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
    virtual void OnHoverRepeat();
    virtual HudUiRect * GetBoundsRectOrNull();
    virtual void OnActivate();
    virtual void OnClearBinding();
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
    virtual int GetX();
    virtual int GetY();
    virtual void EnableWordWrapWithRect(const HudUiRect *rect);
    virtual void GetTextRect(HudUiRect *outRect);
    virtual void SetTextFmt(
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
    virtual void RefreshState();
    virtual int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    virtual void PostLoadFromZrd();
    virtual int OnRawKeyboardChar(int key);
    virtual int OnAcceptForwardToCommit();
    virtual int CommitAndGetValue();
    void SetTimer(float duration);
    void GetRect(HudUiRect *outRect);
    unsigned char HitTestTrue(
        int px,
        int py
    );
};

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
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    int GetCenterX();
    int GetCenterY();
    int HitTest(
        int px,
        int py
    );
    RECOIL_NO_GS void RebuildBltRectFromImage();
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
    HudUiMeter *target,
    int xBase,
    int yBase,
    const int *offsetXY,
    HudUiRect *outRect
);
} // namespace HudUiLayoutNode

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

    void Destructor();
};

extern HudUiMgrSensorBlock g_HudUiMgrSensorBlock;
extern HudUiRect g_HudUiMgrSensorFxRect;
extern int g_HudUiMgrSensorFxViewportWidth;
extern int g_HudUiMgrSensorFxViewportHeight;
extern HudUiRect g_HudUiMgrHudRect;
extern HudUiRect g_HudUiMgrViewRect;
extern float g_HudUiMgrHudRectW;
extern float g_HudUiMgrHudRectH;
extern float g_HudUiMgrReticleMapBiasX;
extern float g_HudUiMgrReticleMapBiasY;
extern float g_HudUiMgrReticleMapScaleHalfW;
extern float g_HudUiMgrReticleMapScaleHalfH;
extern int g_HudUiMgrReticleSnapRadiusSq;
extern float g_HudUiMgrReticleProjection[3];
extern int g_HudUiMgrReticleWidgetHalfW;
extern int g_HudUiMgrReticleWidgetHalfH;
extern int g_HudUiMgrReticleProjectedX;
extern int g_HudUiMgrReticleProjectedY;
extern int g_HudUiMgrReticleMode;
extern HudUiWidget g_HudUiMgrReticleWidget;

struct HudUiContainer {
    HudUiContainer();

    virtual void UpdateAll(float deltaSeconds);
    virtual void SetEnabled(int enabled);

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

    static void Shutdown_Stub();
    void Destructor();
    int SetActive(int active);
    virtual void UpdateAll(float deltaSeconds);
    virtual void LayoutPreUpdate();
    virtual void Enable();
    virtual void Disable();
    virtual void OnActivated();
    void LoadTypeIFromZarRoot(zReader::Node *parentNode);
};

struct HudLayoutSW : HudLayoutBase {
    static HudLayoutSW *GlobalInit();
    static void RegisterAtExit();
    static void AtExitDestructor();
    HudLayoutSW * Constructor();
    void GlobalDestructor();
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

    static void CrtInitGlobalSingleton();
    static HudLayoutHW *GlobalInit();
    static void RegisterAtExit();
    static void AtExitDestructor();
    HudLayoutHW * Constructor();
    void GlobalDestructor();
    virtual void UpdateAll(float deltaSeconds);
    int LoadTypeIIFromZarRoot(zReader::Node *parentNode);
    void ReleaseImages();
    virtual int SetActive(int active);
    virtual void OnActivated();
    virtual void UpdateObjectiveDirtyRect();
    virtual void Enable();
    virtual void Disable();
};

extern HudLayoutHW g_HudLayoutHW;
extern HudLayoutSW g_HudLayoutSW;

extern HudLayoutBase *g_HudUiMgrCurrentLayout;
extern HudUiTextStack4 *g_HudUiTopMessageStack;
extern HudUiTextStack4 *g_HudUiChatMessageStack;

extern HudUiShieldMessageWidget *g_HudUiMgrShieldMessageWidget;

namespace HudLayout {
int __fastcall ApplyViewportRect(HudUiRect *activeRect);
}

namespace HudUiMgrSensor {
void TrackList_Reset();
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
HudUiContainer *__fastcall Constructor(HudUiContainer *self);
void StaticInitAndRegisterAtExit();
HudUiContainer *StaticInit();
void RegisterAtExit();
void AtExitDestructor();
void __fastcall StaticDestructor(HudUiContainer *self);
int __fastcall ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint,
    zVec3 *projectedPoint
);
void __fastcall ScreenToWorld(float *pointXY);
void TriggerCurrentLayoutOnActivated();
int TickLayoutDelay();
int IsLocalPlayerFirstInStatsList();
void __fastcall SetNanitePanelCount(int count);
void __fastcall SetModeCounterState(
    int counterIndex,
    int state
);
void ReticleStaticAtexitStub();
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

extern int g_HudUiMgrObjectivePhase;
extern int g_HudUiMgrObjectiveState;
extern int g_HudUiMgrObjectiveChatComposeActive;
extern float g_HudUiMgrObjectivePhaseTimerSec;
extern float g_HudUiMgrObjectivePhaseDurationSec;
extern float g_HudUiMgrObjectiveAutoHideDelaySec;
extern int g_HudUiMgrObjectiveShowResetUnused;
extern int g_HudUiMgrObjectiveWidgetRightX;
extern float g_HudUiMgrObjectiveMeterFillAnimTimerSec;
extern unsigned int g_HudUiMgrObjectiveMeterFillAnimEnabled;
struct HudUiObjectiveBar;
extern HudUiWidget g_HudUiMgrSensorPanel;
extern HudUiWidget g_HudUiMgrObjectiveWidget;
extern HudUiObjectiveBar g_HudUiMgrObjectiveBar;
extern HudUiWidget g_HudUiMgrObjectiveSensorRect;
extern HudUiWidget g_HudUiMgrSensorOverlay;
extern HudUiMeter g_HudUiMgrSensorMeter;
extern HudUiPanel *g_HudUiMgrObjectiveSummaryTextPanel;
extern HudUiPanel *g_HudUiMgrObjectiveDescTextPanel;
extern HudUiPanel *g_HudUiMgrObjectiveLabelTextPanel;
extern HudUiCounterTextPanel *g_HudUiMgrObjectiveCounterTextPanel;
struct HudUiChatComposeTextInput;
extern HudUiChatComposeTextInput g_HudUiMgrObjectiveChatComposeTextInput;
extern HudUiBar g_HudUiMgrTailBar;
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

struct HudUiTextLabel : HudUiElement {
    char textBuffer[0x100];
    int fontHandle;
    int centerText;
    int centerBoundsLeft;
    int centerBoundsRight;
    int alignMode;

    HudUiTextLabel() {
    }
    HudUiTextLabel(
        const char *text,
        int x,
        int y,
        int flags
    );
    HudUiTextLabel * ConstructorWithPosAndFlags(
        const char *text,
        int x,
        int y,
        int flags
    );
    HudUiTextLabel * CopyConstructor(const HudUiTextLabel *source);
    HudUiTextLabel * Constructor(const HudUiTextLabel *source);
    void SetTextFmt(
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

struct HudUiCircle : HudUiElement {
    int radius;
    int radiusSquared;
    unsigned int color565;

    HudUiCircle * Constructor(
        int x,
        int y,
        int circleRadius,
        unsigned int circleColor565
    );
    void DrawDirty();
    void DrawDirtyForwarder();
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
    void DestructorCore();
    void SetImageByPathOwnedAndRefresh(const char *imagePath);
    void SetImageBorrowedAndRefreshIfChanged(zVidImagePartial *image);
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
    void RebuildBltRect();
};

struct HudUiBackgroundMemberCursorWidget : HudUiBackgroundCursorWidget {
    HudUiBackgroundMemberCursorWidget(
        const char *imagePath,
        int captureEnabled
    ) : HudUiBackgroundCursorWidget(
            imagePath,
            captureEnabled
        ) {
    }
};

struct HudUiPanelPtrVector {
    // Recovered VC5 std::vector<HudUiPanel *> storage. Native builds keep the
    // ABI-compatible record because host STL vector layout is not compatible
    // with the retail VC5 object layout.
    char allocatorProxy;
    char padding[3];
    HudUiPanel **begin;
    HudUiPanel **end;
    HudUiPanel **capacityEnd;

    HudUiPanelPtrVector() {
        // VC5 copies an uninitialized STL allocator proxy byte into the vector.
#if defined(_MSC_VER) && _MSC_VER < 1200
        char allocatorProxyValue;
#else
        char allocatorProxyValue = 0;
#endif
        allocatorProxy = allocatorProxyValue;
        begin = 0;
        end = 0;
        capacityEnd = 0;
    }

    HudUiPanel ** EraseRange(
        HudUiPanel **first,
        HudUiPanel **last
    );
    void InsertN(
        HudUiPanel **position,
        unsigned int count,
        HudUiPanel **valueSource
    );
};

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
    HudUiZrdWidget * Constructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiZrdWidget * ScalarDeletingDestructorThunk(unsigned int flags);
    void DestructorCore();
    void Invalidate();
    HudUiRect * GetBoundsRectOrNull();
    void ShowPreview();
    void OnActivate();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void RefreshState();
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
    HudUiCheckToggleWidget * Constructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiCheckToggleWidget * ScalarDeletingDestructorThunk(unsigned int flags);
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
    HudUiCycleSelectorWidget * Constructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiCycleSelectorWidget * ScalarDeletingDestructorThunk(unsigned int flags);
    void DestructorCore();
    void DestructorCoreThunk();
    void AdvanceSelectionAndActivate();
    void SetIndexClamped(int index);
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
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    void DestructorCore();
    void DestructorCoreThunk();
    void Draw();
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void UpdateNormalizedFromCursor();
    void SetNormalizedValue(float value);
    void SetNormalizedValueAndRebuild(float value);
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
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
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
    HudUiZrdWidgetEx17C * Constructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiZrdWidgetEx17C * ScalarDeletingDestructorThunk(unsigned int flags);
    void DestructorCore();
    void SetVisible(int childIndex);
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void EnableChildAtIndex(int childIndex);
    int SetSelectedIndex(int index);
};

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

    HudUiPanel() {
    }
    HudUiPanel(
        const char *text,
        int x,
        int y
    );
    HudUiPanel * ConstructorDefault(
        const char *text,
        int x,
        int y
    );
    HudUiPanel * ConstructorDefaultThunk();
    HudUiPanel * CopyConstructCore(const HudUiPanel *source);
    HudUiPanel * ConstructorCopy(const HudUiPanel *source);
    void SetClip(
        void *bltSourceOrNull,
        const HudUiRect *rectOrNull
    );
    void Invalidate();
    HGDIOBJ GetFont();
    void SetFontHandle(HGDIOBJ fontHandle);
    void EnableWordWrapWithRect(const HudUiRect *rect);
    void Draw();
    void Destructor();
    void DestructorThunk();
    static void __stdcall DestructorCallback(HudUiPanel *panel);
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
    void SetFont(
        const char *faceName,
        int height,
        int weight,
        int width,
        int italic,
        int charSet,
        int pitchAndFamily
    );
    void SetTextFmt(
        const char *format,
        ...
    );
    void SetTextFmtV(
        const char *format,
        va_list args
    );
    void SetText(const char *text);
    void RebuildTextRect();
    void UpdateTextBoundsFromContent();
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
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiListSelectorItem : HudUiPanel {
    int entryIndex;
    void *owner;

    // Reimplements 0x4b92a0: HudUiListSelectorItem::HudUiListSelectorItem
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

struct HudCmdBindingVector {
    unsigned char allocator;
    unsigned char padding_01[3];
    void *begin;
    void *end;
    void *capacity;

    HudCmdBindingVector() {
#if defined(_MSC_VER) && _MSC_VER < 1200
        char allocatorValue;
#else
        char allocatorValue = 0;
#endif
        allocator = (unsigned char)(allocatorValue);
        begin = 0;
        end = 0;
        capacity = 0;
    }
};

struct HudCmdBindingEntry {
    char *displayText;
    int commandId;

    ~HudCmdBindingEntry();
    static HudCmdBindingEntry *__stdcall DeleteAndReturnNull(HudCmdBindingEntry *entry);
    static HudCmdBindingEntry **__fastcall CopyRange(
        HudCmdBindingEntry **sourceBegin,
        HudCmdBindingEntry **sourceEnd,
        HudCmdBindingEntry **dest
    );
};

struct HudCmdBinding {
    char *displayText;

    static HudCmdBinding **__fastcall DestroyRange(
        HudCmdBinding **first,
        HudCmdBinding **last,
        HudCmdBinding **dest,
        void *unusedAlloc
    );
};

struct HudCmdBindButtonBase : HudUiCheckToggleWidget {
    int bindingSlotTotalCount;
    int visibleBindingSlotCount;
    HudUiListSelectorItem bindPanel;
    HudUiListSelectorItem *bindingSlotPanels;
    HudCmdBindingVector bindingVec;
    int bindingSlotSpacing;
    int selectedBindingIndex;
    float visibleListOffsetX;
    float visibleListOffsetY;
    float overflowListOffsetX;
    float overflowListOffsetY;
    int selectedFontStyleRef;
    int listFontStyleRef;

    HudCmdBindButtonBase();
    int AddBindingEntry(
        const char *displayText,
        int commandId
    );
    void OnSelectedIndexChanged(int selectedIndex);
    void SetSelectedEntry(int selectedIndex);
    void OnSelectionChangedRefresh(int selectedIndex);
    void ClearBindingEntries();
    void DestructorCore();
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

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
};

struct HudCmdKeyAButton : HudCmdBindButtonBase {

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdKeyBButton : HudCmdBindButtonBase {

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdJoyButton : HudCmdBindButtonBase {

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    void OnBeginCapture();
    void OnClearBinding();
};

struct HudCmdMouseButton : HudCmdBindButtonBase {

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    void OnBeginCapture();
    void OnClearBinding();
};

void **__fastcall zUtil_StdPtrVector_Clear(HudCmdBindingVector *self);
void __fastcall zUtil_StdPtrVector_FreeBufferAndReset(
    HudCmdBindingVector *self
);

struct HudUiMessageBoxDialog;

struct HudUiMessageBoxOkButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudUiMessageBoxCancelButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudUiTripletPanel : HudUiElement {
    int visibleCount;
    unsigned char unknown_38[0x04];
    HudUiWidget items[3];

    HudUiTripletPanel * Constructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
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

struct HudUiMessage : HudUiWidget {
    zVidImagePartial *variantImages[5];
    zVidImagePartial *activeSideImages[2];
    zVidImagePartial *sideImageSwaps[2];
    HudUiPanelFull panel;
    HudUiWidget widget;

    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiMessage * Constructor();
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

    HudUiCounter * Constructor();
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

    HudUiBar * Constructor();
    void Draw();
    void SetPointXY(
        int pointIndex,
        float x,
        float y
    );
};

struct HudUiMeter : HudUiBar {
    HudUiMeter * Constructor();
    HudUiMeter * ConstructorEx();
};

struct HudUiObjectiveBar : HudUiBar {};

struct HudUiTextInput {
    virtual void OnPrintableKey(int key);
    virtual void OnIgnoredKey(int key);
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

    HudUiTextInput * Constructor(int bufferSize);
    void AllocTextBuffer(int bufferSize);
    void DestructorCore();
    void DestructorCoreThunk();
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

    virtual void OnAccept();
    void OnAcceptNotifyOwner();
};

struct HudUiChatComposeTextInput : HudUiTextInput {
    virtual void OnAccept();
};

struct HudUiNumericTextInput : HudUiZrdWidget {
    HudUiOwnedTextInput textInput;
    HudUiSliderBorder sliderBorder;

    HudUiNumericTextInput * Constructor(unsigned int maxDigits);
    HudUiNumericTextInput * BaseConstructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
    HudUiNumericTextInput * ScalarDeletingDestructorThunk(unsigned int flags);
    void Destructor();
    void DestructorThunk();
    void AllocTextBuffer(unsigned int bufferSize);
    char * GetBuffer();
    void Update(const char *text);
    RECOIL_NO_GS void UpdateCaptureUiAndClip(float deltaSeconds);
    int SetInputActive(int active);
    void SetRawKeyboardCapture(int enable);
    int OnRawKeyboardChar(int key);
    int OnAcceptForwardToCommit();
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

struct HudUiNetGameSetupOverlayOwner : RecoilApp_IState {
    HudUiNetGameSetupPanel *m_panel;
    int m_reconfigureExistingSession;

    HudUiNetGameSetupOverlayOwner();
    static void StaticInitAndRegisterAtExit();
    static HudUiNetGameSetupOverlayOwner *StaticInit();
    static void RegisterAtExit();
    static void AtExitDestructor();
    ~HudUiNetGameSetupOverlayOwner();
    int OnTryBecomeCurrent();
    void OnDeactivate();
    static void QueueEnterWithReconfigureFlag(int reconfigureExistingSession);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupOverlayOwner) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupOverlayOwner,
        m_panel
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupOverlayOwner,
        m_reconfigureExistingSession
    ) == 0x08
);

struct HudUiClampedIntTextInput : HudUiNumericTextInput {
    int minValue;
    int maxValue;

    HudUiClampedIntTextInput * Constructor(unsigned int maxDigits);
    int OnRawKeyboardChar(int key);
    int OnRawKeyboardDigitOnly(int key);
    int CommitAndGetValue();
};

struct HudUiClampedIntStepButton : HudUiZrdWidget {
    HudUiClampedIntTextInput *targetInput;
    int stepDelta;

    void OnActivate();
};

// BN constructors at 0x40db20 initialize HudUiElement at object offset zero,
// then construct the slot and marker widgets at offsets 0x48 and 0x104.
struct HudUiSlot : HudUiElement {
    unsigned int screenEdgeCode;
    void *trackNode;
    float screenX;
    float screenY;
    unsigned char unknown_44[0x04];
    HudUiWidget slotWidget;
    HudUiWidget trackMarkerWidget;

    HudUiSlot * Constructor();
    void Destructor();
    void Draw();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
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
    HudUiMeter objectiveMeter;
    float objectiveMeterFillAnimTimerSec;
    unsigned int objectiveMeterFillAnimEnabled;
    HudUiPanel *objectiveDescTextPanel;
    HudUiObjectiveBar objectiveBar;
    HudUiTextInput chatComposeTextInput;
    HudUiCounterTextPanel *counterTextPanel;

    void Destructor();
};

struct HudUtil {
    void *fieldPtr;

    void FreeFieldPtr();
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
    void TickFlash(float deltaSeconds);
    void ResetFlashState(float flashRate);
    void SetFlashRate(float flashRate);
    void SetFlashColorAndRate(
        unsigned int flashColor,
        float flashRate
    );
};

struct HudUiFlashPanel {
    static unsigned int __fastcall ComputeFlashBlendColor(
        unsigned int color0,
        unsigned int color1,
        float blend
    );
};

struct HudUiCompositePanelEntry {
    HudUiTransitionTextPanel panel;

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

struct HudUiCompositePanelVector {
    unsigned int allocatorStorage;
    HudUiCompositePanelEntry *begin;
    HudUiCompositePanelEntry *end;
    HudUiCompositePanelEntry *capacityEnd;

    void Clear();
    void InsertCopies(
        HudUiCompositePanelEntry *insertPos,
        unsigned int insertCount,
        const HudUiCompositePanelEntry *templateEntry
    );
};

struct HudUiCompositePanel : HudUiPanel {
    int activeEntryCount;
    HudUiCompositePanelVector entryVector;

    HudUiCompositePanel * ConstructorWithEntryCount(int entryCount);
    void LayoutEntries(
        int x,
        int y
    );
    void ResizeEntryVectorAndRelayout(int entryCount);
    void ReapplyEntryCount();
    void ResizeEntryCount(
        int oldCount,
        int entryCount
    );
    void SetTextFmt(
        const char *format,
        ...
    );
    void SetTextFmtV(
        const char *format,
        va_list args
    );
    void ScrollHistory();
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
    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
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
    ~HudUiBackground();
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
    virtual void Update(float deltaSeconds);
};

struct HudCmdSimpleWidget : HudUiZrdWidget {
};

struct HudCmdNextSetButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudCmdPrevSetButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudCmdNextCommandButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudCmdPrevCommandButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudCmdResetButton : HudUiZrdWidget {

    void OnActivate();
};

struct HudCmdSetListWidget : HudUiCycleSelectorWidget {

    void OnActivate();
};

struct HudCmdPromptPanel : HudUiTransitionTextPanel {
};

struct HudCmdDescriptionPanel : HudUiPanel {
    int captureState;
};

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
    HudCmdPromptPanel promptPanel;
    HudCmdDescriptionPanel descriptionPanel;

    HudCmdDialog * Constructor();
    void Destructor();
    HudCmdDialog * ScalarDeletingDestructor(unsigned int flags);
    void UpdateCaptureState(float deltaTime);
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

struct HudUiOptionsPanelBackButton : HudUiZrdWidget {
    HudUiOptionsPanelBackButton() {
    }

    void OnActivate();
};

struct HudUiOptionsPanel_Lighting : HudUiCheckToggleWidget {
    HudUiOptionsPanel_Lighting() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_Perspective : HudUiCheckToggleWidget {
    HudUiOptionsPanel_Perspective() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_FullHud : HudUiCheckToggleWidget {
    HudUiOptionsPanel_FullHud() {
    }

    void PostLoadFromZrd();
    void InitFromOptions();
};

struct HudUiOptionsPanel_ObjectDetail : HudUiCycleSelectorWidget {
    HudUiOptionsPanel_ObjectDetail() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_TextureMemory : HudUiCycleSelectorWidget {
    HudUiOptionsPanel_TextureMemory() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_Effects : HudUiCycleSelectorWidget {
    HudUiOptionsPanel_Effects() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundActive : HudUiCheckToggleWidget {
    HudUiOptionsPanel_SoundActive() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundQuality : HudUiCycleSelectorWidget {
    HudUiOptionsPanel_SoundQuality() {
    }

    void OnActivate();
    void PostLoadFromZrd();
    void InitFromOptions();
    void SyncFromOptions();
};

struct HudUiOptionsPanel_SoundVolume : HudUiFillBitmap {
    HudUiOptionsPanel_SoundVolume() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_MusicEnable : HudUiCheckToggleWidget {
    HudUiOptionsPanel_MusicEnable() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_MusicVolume : HudUiFillBitmap {
    HudUiOptionsPanel_MusicVolume() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

struct HudUiOptionsPanel_Resolution : HudUiCycleSelectorWidget {
    HudUiOptionsPanel_Resolution() {
    }

    void PostLoadFromZrd();
    void SyncFromOptions();
    void OnActivate();
};

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
    void DestructorCore();
    HudOptionsDialog * ScalarDeletingDestructor(unsigned int flags);
};

struct HudCmdDialogState : RecoilApp_IState {
    HudCmdDialog *m_dialog;

    HudCmdDialogState();
    static void StaticInitAndRegisterAtExit();
    static HudCmdDialogState *StaticInit();
    static void RegisterAtExit();
    static void AtExitDestructor();
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

extern HudCmdDialogState g_HudCmdDialogState;

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
    HudUiMessageBoxDialog * ScalarDeletingDestructor(unsigned int flags);
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

    HudUiPanelLayoutEntry * CopyConstruct(const HudUiPanelLayoutEntry *source);
    HudUiPanelLayoutEntry * CopyAssign(const HudUiPanelLayoutEntry *source);
    static HudUiPanelLayoutEntry *__fastcall CopyAssignRange(
        const HudUiPanelLayoutEntry *sourceStart,
        const HudUiPanelLayoutEntry *sourceEnd,
        HudUiPanelLayoutEntry *dest
    );
    static void __stdcall DestroyRange(
        HudUiPanelLayoutEntry *start,
        HudUiPanelLayoutEntry *end
    );
};

struct HudUiPanelSpan {
    unsigned int allocatorProxy;
    HudUiPanelLayoutEntry *begin;
    HudUiPanelLayoutEntry *end;
    HudUiPanelLayoutEntry *cap;

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
    HudUiPanelSpan *begin;
    HudUiPanelSpan *end;
    HudUiPanelSpan *cap;

    HudUiPanelSpanVec() {
#if defined(_MSC_VER) && _MSC_VER < 1200
        char allocatorProxyValue;
#else
        char allocatorProxyValue = 0;
#endif
        allocatorProxy = allocatorProxyValue;
        begin = 0;
        end = 0;
        cap = 0;
    }
    void InsertN(
        HudUiPanelSpan *insertPos,
        unsigned int count,
        const HudUiPanelSpan *templateSpan
    );
};

struct HudUiCreditsBackButton : HudUiZrdWidget {
    HudUiCreditsBackButton();
    ~HudUiCreditsBackButton();
    void OnActivate();
};

struct HudUiCreditsQuitButton : HudUiZrdWidget {
    HudUiCreditsQuitButton();
    ~HudUiCreditsQuitButton();
    void OnActivate();
};

struct HudUiZrdScrollingText : HudUiZrdWidget {
    HudUiPanelSpanVec rows;
    HudUiRect rect;
    int totalHeight;

    HudUiZrdScrollingText();
    ~HudUiZrdScrollingText();
    void OnActivate();
    void OnActivateResetOwnerFade();
    void Update(float deltaSeconds);
    void UpdateScrollPositions(float scrollProgress);
    int LoadFromZrd(
        zReader::Node *zrdSection,
        HudUiBackground *ownerDialog
    );
    void Destructor();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiCreditsPanel : HudUiBackground {
    float fadeStep;
    HudUiCreditsBackButton backButton;
    HudUiCreditsQuitButton quitButton;
    HudUiZrdScrollingText creditsScreen;
    float fadeProgress;

    HudUiCreditsPanel();
    void Update(float deltaSeconds);
    void UpdateFadeAndExit(float deltaSeconds);
    void Destructor();
    HudUiCreditsPanel * ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiPanelSimple : HudUiPanel {
    HudUiPanelSimple * Constructor(
        const char *text,
        int x,
        int y
    );
    HudUiPanelSimple * ConstructorDefaultThunk();
};

struct HudUiShieldMessageWidget {
    int state;
    int viewportResetFrame;
    unsigned char unknown_08[0x04];
    HudUiRect screenRect;
    HudUiWidget widget;
    HudUiPanelSimple percentTextPanel;
    HudUiMeter meter;
    unsigned char unknown_4bc[0x08];

    static int __stdcall ApplyLayout(zReader::Node *layoutRoot);
    void Destructor();
};

typedef HudUiShieldMessageWidget HudUiShieldMessageWidgetState;

struct HudUiTimerPanelFloat : HudUiPanel {
    float sampleElapsedSec;
    float displayValue;
    float sampleFrameCount;

    HudUiTimerPanelFloat * ConstructorDefault();
    void Draw();
};

struct HudUiStringMenu : HudUiContainer {
    unsigned char unknown_10[0x10];
    HudUiPanelSimple items[23];

    void DestructorCore();
};

struct HudUiStatsListElement : HudUiElement {
    HudUiTriplet *triplet;

    void Update(float deltaSeconds);
    void DestructorCore();
    HudUiElement * ScalarDeletingDestructor(unsigned int flags);
};

struct HudUiTimerPanel : HudUiPanel {
    float elapsedSeconds;
    int stopped;
    int secondsStep;

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
    HudUiTimerPanel * ConstructorDefault();
    void Update(float deltaSeconds);
    void UpdateHMSFromSeconds(float seconds);
    void SetTimeSeconds(
        int hours,
        int minutes,
        int seconds
    );
};

struct HudUiCounterTextPanel : HudUiPanel {
    HudUiCounterTextPanel * Constructor();
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

    HudUiTriplet * Constructor();
    void DestructorCore();
    void InterpolateLayout(float t);
    void RebuildDisplay();
    void AddEntry(GameNetPlayerRow *entryData);
    void UpdateEntryData(GameNetPlayerRow *entryData);
    void RemoveEntry(GameNetPlayerRow *entryKey);
    int IsLocalPlayerFirstEntry();
};

struct HudUiTextStack4 : HudUiContainer {
    unsigned char lines[4][0x2a4];

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

struct HudUiTopMessageStack : HudUiTextStack4 {
    HudUiTopMessageStack * Constructor();
    void DestructorCore();
};

struct HudUiChatMessageStack : HudUiTextStack4 {
    HudUiChatMessageStack * Constructor();
    void DestructorCore();
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(HudUiRect) == 0x10);
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
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelPtrVector) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelPtrVector,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelPtrVector,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelPtrVector,
        capacityEnd
    ) == 0x0c
);
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
RECOIL_STATIC_ASSERT(sizeof(HudCmdBindingVector) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudCmdBindingVector,
        capacity
    ) == 0x0c
);
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
RECOIL_STATIC_ASSERT(sizeof(HudCmdPromptPanel) == 0x2c0);
RECOIL_STATIC_ASSERT(sizeof(HudCmdDescriptionPanel) == 0x2a8);
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
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpan,
        cap
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(HudUiPanelSpanVec) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiPanelSpanVec,
        cap
    ) == 0x0c
);
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
        HudUiCompositePanelVector,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCompositePanelVector,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiCompositePanelVector,
        capacityEnd
    ) == 0x0c
);
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
RECOIL_STATIC_ASSERT(sizeof(HudUiMeter) == 0x140);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMeter,
        points
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMeter,
        drawVertexCount
    ) == 0x130
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMeter,
        color565
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMeter,
        fillPixelsMax
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMeter,
        meterFlags
    ) == 0x13c
);
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
