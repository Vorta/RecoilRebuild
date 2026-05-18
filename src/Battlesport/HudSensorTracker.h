#pragma once

#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "recoil/recoil_callconv.h"

#include <stddef.h>
#include "recoil/recoil_types.h"
#include <stdio.h>

struct zSndSample;
struct OptCatalogEntryDef;
struct HudSensorTracker;

struct HudSensorMapPoint {
    float x;
    float y;
    float z;
};

struct HudSensorMapBounds {
    float minX;
    float minY;
    float minZ;
    float maxX;
    float maxY;
    float maxZ;
};

struct HudRectI {
    int left;
    int top;
    int right;
    int bottom;

    RECOIL_NOINLINE int RECOIL_THISCALL CalcOutcode(const zVec3 *point);
    RECOIL_NOINLINE int RECOIL_THISCALL SegmentIntersectsEdge(int edgeCode,
                                                                       const zVec3 *segmentStart,
                                                                       const zVec3 *segmentEnd);
    RECOIL_NOINLINE int RECOIL_THISCALL ClipOrSplitSegment(zVec3 *segmentStart,
                                                                    zVec3 *segmentEnd);
    RECOIL_NOINLINE static int RECOIL_FASTCALL IsCornerOutcode(int outcode);
};

struct HudSensorMapNode {
    HudSensorMapNode *next;
    char colorRgb[4];
    int pointCount;
    HudSensorMapPoint *points;
    int objectiveIndex;
    int selectedPointIndex;
    int isEnabled;
    float blinkTimerSec;
    int packedColor565Pair;
    HudSensorMapBounds cachedBounds;

    RECOIL_NOINLINE HudSensorMapNode *RECOIL_THISCALL Init();
    RECOIL_NOINLINE void RECOIL_THISCALL FreePointArray();
    RECOIL_NOINLINE int RECOIL_THISCALL SetEnabled(int enabled);
    RECOIL_NOINLINE HudSensorMapPoint *RECOIL_THISCALL SelectPoint(int pointIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL InitDefaults();
    RECOIL_NOINLINE int RECOIL_THISCALL SetColorRgb(const unsigned char *rgbOrNull);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadFromStream(FILE *stream);
    RECOIL_NOINLINE int RECOIL_THISCALL
    UpdateCachedBounds(HudSensorMapBounds *outBoundsOrNull);
    RECOIL_NOINLINE int RECOIL_THISCALL DrawOnTracker(HudSensorTracker *tracker,
                                                               const zVec3 *drawPathWorldPos);
    RECOIL_NOINLINE int RECOIL_THISCALL DrawProjectedPath(HudSensorTracker *tracker);
};

struct HudSensorObjectiveSlot {
    int completedFlag;
    int autoplayFlag;
    zClass_NodePartial *activationNode;
    zClass_NodePartial *inactivationNode;
    zVidImagePartial *objectiveImage;
    char objectiveTitle[0x100];
    char objectiveDesc[0x100];
    char objectiveSummary[0x100];
    int objectiveReadFlag;
    zSndSample *readSoundSample;

    RECOIL_NOINLINE void RECOIL_THISCALL Reset();
};

struct HudSensorPendingPlayerSave {
    PlayerMissionSaveData playerSaveData;
    int savedNanitePanelLevel;
    int skipTimerResetOnStart;
};

struct HudSensorTracker {
    HudUiRect outerRect;
    HudUiRect innerRectExpanded;
    int mapFileVersion;
    int mapHeaderDword;
    float mapBoundsMinX;
    int unknown_2c;
    float mapBoundsMinZ;
    float mapBoundsMaxX;
    int unknown_38;
    float mapBoundsMaxZ;
    HudSensorMapNode *mapNodeListHead;
    int mapLoadedFlag;
    zUtil_SaveGameState *trackedSaveStateSelection;
    int outerRectCenterY;
    int mapOverlayCenterX;
    int mapOverlayCenterY;
    char *loadedMapPath;
    zSndSample *mapSndOn;
    zSndSample *mapSndOff;
    zSndSample *mapSndClick;
    int mapScaleLerpActive;
    zVec3 *trackedWorldPosPtr;
    int mapScaleLerpRunning;
    zVec3 *trackedWorldOriginPtr;
    zVec3 *trackedForwardVecPtr;
    zVec3 trackedForwardFallbackVec;
    zVec3 trackedWorldFallbackOrigin;
    zVec3 mapScaleCurrent;
    float mapScaleLerpT;
    float mapZoom;
    zVec3 mapScaleStart;
    zVec3 mapScaleGoal;
    unsigned char unknown_c0[0x08];
    float saveStateMarkerMaxDistSq;
    float mapScaleLerpStep;
    int objectiveUiMode;
    float hudScale;
    int missionLoaded;
    int missionId;
    int missionFlags;
    CString missionDataPath;
    CString zbdPath;
    CString missionGsPath;
    zClass_NodePartial *worldNode;
    zClass_NodePartial *cameraNode;
    zClass_NodePartial *windowNode;
    zClass_NodePartial *displayNode;
    float objectiveMeterSeconds;
    int objectiveReadTimeSecRaw;
    int objectiveFlowState;
    zSndSample *currentObjectiveReadSound;
    zSndSample *objectiveIncomingSfx;
    int currentObjectiveIndex;
    zSndSample *objectiveReviewSfx;
    int objectiveReviewDelaySecRaw;
    zReader::Node *objectivesRootNode;
    int firstIncompleteObjectiveIndex;
    zSndSample *objectiveCompleteSfx;
    int objectiveCount;
    int objectiveReadSoundDelaySecRaw;
    int objectiveFlowDeadlineSecRaw;
    int completedObjectiveCount;
    char objectiveSummaryText[0x400];
    HudSensorObjectiveSlot objectiveSlots[10];
    int missionStat0;
    int missionStat1;
    int primaryGunDispatchCount;
    int missionStat3;
    int weaponsFoundMask;
    int raceCheckpointMode;
    int checkpointCount;
    int runtimeGoalValue;
    int runtimeTimerSecRaw;
    HudUiElement *fxPass3Obj;
    int finalMissionFlag;
    float menuTransitionDelaySec;
    int hasPendingPlayerSave;
    HudSensorPendingPlayerSave pendingPlayerSave;

    RECOIL_NOINLINE void RECOIL_THISCALL Init(const HudUiRect *outerRectOrNull);
    RECOIL_NOINLINE HudSensorTracker *RECOIL_THISCALL InitNoBounds();
    RECOIL_NOINLINE HudSensorTracker *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Shutdown();
    RECOIL_NOINLINE void RECOIL_THISCALL SetBounds(const HudUiRect *outerRect,
                                                   const HudUiRect *innerRectOrNull);
    RECOIL_NOINLINE int RECOIL_THISCALL
    SetTrackedSaveState(zUtil_SaveGameState *saveState);
    RECOIL_NOINLINE int RECOIL_THISCALL SetSaveStateMarkerMaxDistance(float maxDist);
    RECOIL_NOINLINE void RECOIL_THISCALL MapOverlayEndShow();
    RECOIL_NOINLINE int RECOIL_THISCALL MapOverlayBeginShow();
    RECOIL_NOINLINE int RECOIL_THISCALL MapOverlayRefToggle(int enable);
    RECOIL_NOINLINE void RECOIL_THISCALL MapZoomIn();
    RECOIL_NOINLINE void RECOIL_THISCALL MapZoomOut();
    RECOIL_NOINLINE int RECOIL_THISCALL UpdateMapScaleLerp();
    RECOIL_NOINLINE void RECOIL_THISCALL Update();
    RECOIL_NOINLINE int RECOIL_THISCALL ProjectWorldPointsToOverlay(
        const zVec3 *inputWorldPoints, zVec3 *projectedOverlayPoints, int pointCount);
    RECOIL_NOINLINE float RECOIL_THISCALL GetSaveStateRelativeVectorLen(
        zUtil_SaveGameState *saveState, zVec3 *relativeDelta, int takeSqrt);
    RECOIL_NOINLINE int RECOIL_THISCALL DrawTrackedSaveStateMarker();
    RECOIL_NOINLINE int RECOIL_THISCALL
    DrawSaveStateMarker(zUtil_SaveGameState *saveState);
    RECOIL_NOINLINE int RECOIL_THISCALL MapRemoveNode(HudSensorMapNode *mapNode);
    RECOIL_NOINLINE int RECOIL_THISCALL
    MapInsertNodeAndGrowBounds(HudSensorMapNode *mapNode);
    RECOIL_NOINLINE int RECOIL_THISCALL MapShutdownAndResetThunk();
    RECOIL_NOINLINE int RECOIL_THISCALL MapShutdownAndReset();
    RECOIL_NOINLINE int RECOIL_THISCALL LoadMapFromStream(FILE *stream);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadMapFromPath(const char *path);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadMissionMapAndSfx(int missionId);
    RECOIL_NOINLINE int RECOIL_THISCALL ResetMissionState();
    RECOIL_NOINLINE void RECOIL_THISCALL RegisterMissionSectionHandlers();
    RECOIL_NOINLINE int RECOIL_THISCALL
    WriteMissionDataSection(zZbdSectionCallbackCtx *writer);
    RECOIL_NOINLINE int RECOIL_THISCALL InitMissionIdAndFlags(int missionId,
                                                                       int flags);
    RECOIL_NOINLINE int RECOIL_THISCALL SetZbdPath(const char *path);
    RECOIL_NOINLINE int RECOIL_THISCALL SetMissionId(int missionId);
    RECOIL_NOINLINE int RECOIL_THISCALL GetMissionId();
    RECOIL_NOINLINE int RECOIL_THISCALL UnloadObjectives();
    RECOIL_NOINLINE int RECOIL_THISCALL LoadObjectivesFromPath(const char *path);
    RECOIL_NOINLINE int RECOIL_THISCALL
    LoadObjectivesFromZrd(int unusedStack = 0);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadRaceCheckpointMeta();
    RECOIL_NOINLINE void RECOIL_THISCALL RunStartAnimsFromZrd(const char *zrdPath,
                                                              const char *namedNodeName);
    RECOIL_NOINLINE int RECOIL_THISCALL
    QueueMissionFmvStateForMissionId(int missionId);
    RECOIL_NOINLINE void RECOIL_THISCALL SaveAndQueueMissionState();
    RECOIL_NOINLINE int RECOIL_THISCALL
    GetObjectiveBriefingStringsAndImageRef(int objectiveIndex, char **outSummary,
                                           char **outDesc, zVidImagePartial **outImageRef);
    RECOIL_NOINLINE int RECOIL_THISCALL SetObjectiveMarkerEnabledAndColor(
        int objectiveIndex, int enabled, const unsigned char *colorRgb24);
    RECOIL_NOINLINE int RECOIL_THISCALL
    SetObjectiveMarkerColorBlink(int objectiveIndex, const unsigned char *colorRgb24);
    RECOIL_NOINLINE int RECOIL_THISCALL FindAndHighlightFirstIncompleteObjective();
    RECOIL_NOINLINE void RECOIL_THISCALL SetRuntimeTimerSecAndGoalValue(int timerSecRaw,
                                                                        int goalValue);
    RECOIL_NOINLINE int RECOIL_THISCALL SetObjectiveReviewVisible(int visible);
    RECOIL_NOINLINE void RECOIL_THISCALL AdvanceObjectiveState();
    RECOIL_NOINLINE void RECOIL_THISCALL ResetHudForMissionStart();
    RECOIL_NOINLINE void RECOIL_THISCALL Command_ToggleObjectivePanel();
    RECOIL_NOINLINE void RECOIL_THISCALL SetObjectivePanelVisible(int visible);
    RECOIL_NOINLINE void RECOIL_THISCALL Command_ShowObjectivePickupInfo();
    RECOIL_NOINLINE void RECOIL_THISCALL ShowObjectivePickupInfo(int visible,
                                                                 int startAutoAdvance,
                                                                 OptCatalogEntryDef *optEntry);
    RECOIL_NOINLINE int RECOIL_THISCALL UpdateObjectiveFlow();

    RECOIL_NOINLINE static int RECOIL_FASTCALL
    ZarMission_SaveCallback(zZbdSectionCallbackCtx *writer, HudSensorTracker *self);
    RECOIL_NOINLINE static int RECOIL_FASTCALL
    ZarMission_RestoreCallback(void *reader, const char *token, const void *missionData,
                               unsigned int dataSize, HudSensorTracker *self);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    ZarMissionLate_SaveCallback(zZbdSectionCallbackCtx *writer, HudSensorTracker *self);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    ZarMissionLate_RestoreCallback(void *reader, const char *token, const void *lateMissionData,
                                   unsigned int dataSize, HudSensorTracker *self);
    RECOIL_NOINLINE static void RECOIL_FASTCALL OnObjectiveReadSoundEvent(int eventCode);
    RECOIL_NOINLINE static void RECOIL_FASTCALL OnObjectiveCommand(int commandId);
    RECOIL_NOINLINE static HudSensorTracker *RECOIL_CDECL ConstructGlobal();
    RECOIL_NOINLINE static void RECOIL_CDECL RegisterGlobalOnExit();
    RECOIL_NOINLINE static void RECOIL_CDECL ShutdownGlobal();
    RECOIL_NOINLINE static void RECOIL_FASTCALL DrawMarkerCross(
        int centerX, int centerY, int armHalfWidth,
        int armHalfHeight, int markerColor, HudSensorTracker *tracker);
    RECOIL_NOINLINE static void RECOIL_FASTCALL
    DrawDiamondMarker(int centerX, int centerY, int halfWidth,
                      int halfHeight, int markerColor, HudSensorTracker *tracker);
};

RECOIL_STATIC_ASSERT(sizeof(HudSensorMapPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudRectI) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapPoint, z) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(HudSensorMapBounds) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapBounds, minZ) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapBounds, maxX) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapBounds, maxZ) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(HudSensorMapNode) == 0x3c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapNode, points) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorMapNode, cachedBounds) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapNodeListHead) == 0x40);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapLoadedFlag) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, loadedMapPath) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapSndOff) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapScaleLerpActive) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, trackedWorldOriginPtr) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, trackedForwardVecPtr) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapScaleCurrent) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapZoom) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapScaleStart) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, mapScaleGoal) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, saveStateMarkerMaxDistSq) == 0xc8);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, missionId) == 0xdc);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, missionFlags) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, zbdPath) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, worldNode) == 0xf0);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, cameraNode) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, displayNode) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, objectiveFlowState) == 0x108);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, currentObjectiveIndex) == 0x114);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, objectivesRootNode) == 0x120);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, firstIncompleteObjectiveIndex) == 0x124);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, objectiveCount) == 0x12c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, objectiveFlowDeadlineSecRaw) == 0x134);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, completedObjectiveCount) == 0x138);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, objectiveSlots) == 0x53c);
RECOIL_STATIC_ASSERT(sizeof(HudSensorObjectiveSlot) == 0x31c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, autoplayFlag) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, activationNode) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, inactivationNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, objectiveImage) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, objectiveReadFlag) == 0x314);
RECOIL_STATIC_ASSERT(offsetof(HudSensorObjectiveSlot, readSoundSample) == 0x318);
RECOIL_STATIC_ASSERT(sizeof(HudSensorPendingPlayerSave) == 0x148);
RECOIL_STATIC_ASSERT(offsetof(HudSensorPendingPlayerSave, skipTimerResetOnStart) == 0x144);
RECOIL_STATIC_ASSERT(offsetof(HudSensorPendingPlayerSave, savedNanitePanelLevel) == 0x140);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, missionStat0) == 0x2454);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, primaryGunDispatchCount) == 0x245c);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, fxPass3Obj) == 0x2478);
RECOIL_STATIC_ASSERT(offsetof(HudSensorTracker, pendingPlayerSave) == 0x2488);
RECOIL_STATIC_ASSERT(sizeof(HudSensorTracker) == 0x25d0);

extern "C" {
extern HudSensorTracker g_HudSensorTracker;

extern "C" {
extern int g_RecoilApp_QuitAfterCredits;
}
}

namespace HudGeom2D {
RECOIL_NOINLINE int RECOIL_FASTCALL ClassifyPointAgainstSegment(const zVec3 *segmentStart,
                                                                         const zVec3 *segmentEnd,
                                                                         const zVec3 *point);
}

namespace HudLineClip {
RECOIL_NOINLINE void RECOIL_FASTCALL SetCurrentBoundsFromRectI(const HudRectI *rect);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipSegmentToCurrentBounds(
    zVec3 *point0, zVec3 *point1, int *point0Clipped, int *point1Clipped);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipSegmentToCurrentXBounds(
    zVec3 *point0, zVec3 *point1, int *point0Clipped, int *point1Clipped);
RECOIL_NOINLINE int RECOIL_FASTCALL ClipSegmentToCurrentYBounds(
    zVec3 *point0, zVec3 *point1, int *point0Clipped, int *point1Clipped);
RECOIL_NOINLINE void RECOIL_FASTCALL ClipEndpointToX(zVec3 *endpoint, const zVec3 *otherEndpoint,
                                                     float clipX);
RECOIL_NOINLINE void RECOIL_FASTCALL ClipEndpointToY(zVec3 *endpoint, const zVec3 *otherEndpoint,
                                                     float clipY);
} // namespace HudLineClip
