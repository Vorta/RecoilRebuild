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

#include "recoil/recoil_types.h"
#include <stddef.h>
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

    int CalcOutcode(const zVec3 *point);
    int SegmentIntersectsEdge(
        int edgeCode,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd
    );
    int ClipOrSplitSegment(
        zVec3 *segmentStart,
        zVec3 *segmentEnd
    );
    static int __fastcall IsCornerOutcode(int outcode);
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

    HudSensorMapNode * Init();
    void FreePointArray();
    int SetEnabled(int enabled);
    HudSensorMapPoint * SelectPoint(int pointIndex);
    int InitDefaults();
    int SetColorRgb(const unsigned char *rgbOrNull);
    int LoadFromStream(FILE *stream);
    int UpdateCachedBounds(HudSensorMapBounds *outBoundsOrNull);
    int DrawOnTracker(
        HudSensorTracker *tracker,
        const zVec3 *drawPathWorldPos
    );
    int DrawProjectedPath(HudSensorTracker *tracker);
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

    void Reset();
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
    zClass_NodePartial *mapWorldNode;
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
    union {
        int missionFlags;
        zClass_NodePartial *effectResourceNode;
    };
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

    void Init(const HudUiRect *outerRectOrNull);
    HudSensorTracker * InitNoBounds();
    HudSensorTracker * Constructor();
    void Shutdown();
    void SetBounds(
        const HudUiRect *outerRect,
        const HudUiRect *innerRectOrNull
    );
    int SetTrackedSaveState(zUtil_SaveGameState *saveState);
    int SetSaveStateMarkerMaxDistance(float maxDist);
    void MapOverlayEndShow();
    int MapOverlayBeginShow();
    int MapOverlayRefToggle(int enable);
    void MapZoomIn();
    void MapZoomOut();
    int UpdateMapScaleLerp();
    void Update();
    int ProjectWorldPointsToOverlay(
        const zVec3 *inputWorldPoints,
        zVec3 *projectedOverlayPoints,
        int pointCount
    );
    float GetSaveStateRelativeVectorLen(
        zUtil_SaveGameState *saveState,
        zVec3 *relativeDelta,
        int takeSqrt
    );
    int DrawTrackedSaveStateMarker();
    int DrawSaveStateMarker(zUtil_SaveGameState *saveState);
    int MapRemoveNode(HudSensorMapNode *mapNode);
    int MapInsertNodeAndGrowBounds(HudSensorMapNode *mapNode);
    int MapShutdownAndResetThunk();
    int MapShutdownAndReset();
    int LoadMapFromStream(FILE *stream);
    int LoadMapFromPath(const char *path);
    int LoadMissionMapAndSfx(int missionId);
    int ResetMissionState();
    void RegisterMissionSectionHandlers();
    int WriteMissionDataSection(zZbdSectionCallbackCtx *writer);
    int ApplyMissionDataAndReload(
        void *reader,
        const char *token,
        const void *missionData,
        unsigned int dataSize
    );
    int InitMissionIdAndFlags(
        int missionId,
        int flags
    );
    int SetZbdPath(const char *path);
    int SetMissionId(int missionId);
    int GetMissionId();
    int LoadMissionCoreResources();
    int InitMissionGameplaySystems();
    int ShutdownMissionGameplaySystems();
    void LoadMissionWeatherFx(const char *zrdPath);
    int UnloadObjectives();
    int LoadObjectivesFromPath(const char *path);
    int LoadObjectivesFromZrd(const char *zrdPath = 0);
    int LoadRaceCheckpointMeta();
    void RunStartAnimsFromZrd(
        const char *zrdPath,
        const char *namedNodeName
    );
    int QueueMissionFmvStateForMissionId(int missionId);
    void SaveAndQueueMissionState();
    int GetObjectiveBriefingStringsAndImageRef(
        int objectiveIndex,
        char **outSummary,
        char **outDesc,
        zVidImagePartial **outImageRef
    );
    int SetObjectiveMarkerEnabledAndColor(
        int objectiveIndex,
        int enabled,
        const unsigned char *colorRgb24
    );
    int SetObjectiveMarkerColorBlink(
        int objectiveIndex,
        const unsigned char *colorRgb24
    );
    int FindAndHighlightFirstIncompleteObjective();
    void SetRuntimeTimerSecAndGoalValue(
        int timerSecRaw,
        int goalValue
    );
    int SetObjectiveReviewVisible(int visible);
    void AdvanceObjectiveState();
    void ResetHudForMissionStart();
    void Command_ToggleObjectivePanel();
    void SetObjectivePanelVisible(int visible);
    void Command_ShowObjectivePickupInfo();
    void ShowObjectivePickupInfo(
        int visible,
        int startAutoAdvance,
        OptCatalogEntryDef *optEntry
    );
    int UpdateObjectiveFlow();

    static int __fastcall ZarMission_SaveCallback(
        zZbdSectionCallbackCtx *writer,
        HudSensorTracker *self
    );
    static int __fastcall ZarMission_RestoreCallback(
        void *reader,
        const char *token,
        const void *missionData,
        unsigned int dataSize,
        HudSensorTracker *self
    );
    static void __fastcall ZarMissionLate_SaveCallback(
        zZbdSectionCallbackCtx *writer,
        HudSensorTracker *self
    );
    static void __fastcall ZarMissionLate_RestoreCallback(
        void *reader,
        const char *token,
        const void *lateMissionData,
        unsigned int dataSize,
        HudSensorTracker *self
    );
    static void __fastcall OnObjectiveReadSoundEvent(int eventCode);
    static void __fastcall OnObjectiveCommand(int commandId);
    static HudSensorTracker *ConstructGlobal();
    static void RegisterGlobalOnExit();
    static void ShutdownGlobal();
    static void __fastcall DrawMarkerCross(
        int centerX,
        int centerY,
        int armHalfWidth,
        int armHalfHeight,
        int markerColor,
        HudSensorTracker *tracker
    );
    static void __fastcall DrawDiamondMarker(
        int centerX,
        int centerY,
        int halfWidth,
        int halfHeight,
        int markerColor,
        HudSensorTracker *tracker
    );
    static int __fastcall ParseCheckpointNumberFromNode(
        zClass_NodePartial *node
    );
};

RECOIL_STATIC_ASSERT(sizeof(HudSensorMapPoint) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(HudRectI) == 0x10);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapPoint,
        z
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(HudSensorMapBounds) == 0x18);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapBounds,
        minZ
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapBounds,
        maxX
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapBounds,
        maxZ
    ) == 0x14
);
RECOIL_STATIC_ASSERT(sizeof(HudSensorMapNode) == 0x3c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapNode,
        points
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorMapNode,
        cachedBounds
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapNodeListHead
    ) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapLoadedFlag
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        loadedMapPath
    ) == 0x58
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapSndOff
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapScaleLerpActive
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        trackedWorldOriginPtr
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        trackedForwardVecPtr
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapScaleCurrent
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapZoom
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapScaleStart
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        mapScaleGoal
    ) == 0xb4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        saveStateMarkerMaxDistSq
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        missionId
    ) == 0xdc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        missionFlags
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        zbdPath
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        worldNode
    ) == 0xf0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        cameraNode
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        displayNode
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        objectiveFlowState
    ) == 0x108
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        currentObjectiveIndex
    ) == 0x114
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        objectivesRootNode
    ) == 0x120
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        firstIncompleteObjectiveIndex
    ) == 0x124
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        objectiveCount
    ) == 0x12c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        objectiveFlowDeadlineSecRaw
    ) == 0x134
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        completedObjectiveCount
    ) == 0x138
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        objectiveSlots
    ) == 0x53c
);
RECOIL_STATIC_ASSERT(sizeof(HudSensorObjectiveSlot) == 0x31c);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        autoplayFlag
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        activationNode
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        inactivationNode
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        objectiveImage
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        objectiveReadFlag
    ) == 0x314
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorObjectiveSlot,
        readSoundSample
    ) == 0x318
);
RECOIL_STATIC_ASSERT(sizeof(HudSensorPendingPlayerSave) == 0x148);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorPendingPlayerSave,
        skipTimerResetOnStart
    ) == 0x144
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorPendingPlayerSave,
        savedNanitePanelLevel
    ) == 0x140
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        missionStat0
    ) == 0x2454
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        primaryGunDispatchCount
    ) == 0x245c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        fxPass3Obj
    ) == 0x2478
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudSensorTracker,
        pendingPlayerSave
    ) == 0x2488
);
RECOIL_STATIC_ASSERT(sizeof(HudSensorTracker) == 0x25d0);

union HudSensorTrackerStorage {
    unsigned long align;
    unsigned char bytes[sizeof(HudSensorTracker)];
};
RECOIL_STATIC_ASSERT(sizeof(HudSensorTrackerStorage) == 0x25d0);

extern "C" {
extern HudSensorTrackerStorage g_HudSensorTracker;
#define g_HudSensorTracker \
    (*(HudSensorTracker *)&g_HudSensorTracker)
extern char g_HudSensor_MissionSoundSetName[0x20];

extern "C" {
extern int g_RecoilApp_QuitAfterCredits;
}
}

namespace HudGeom2D {
int __fastcall ClassifyPointAgainstSegment(
    const zVec3 *segmentStart,
    const zVec3 *segmentEnd,
    const zVec3 *point
);
}

namespace HudLineClip {
void __fastcall SetCurrentBoundsFromRectI(const HudRectI *rect);
int __fastcall ClipSegmentToCurrentBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
);
int __fastcall ClipSegmentToCurrentXBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
);
int __fastcall ClipSegmentToCurrentYBounds(
    zVec3 *point0,
    zVec3 *point1,
    int *point0Clipped,
    int *point1Clipped
);
void __fastcall ClipEndpointToX(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipX
);
void __fastcall ClipEndpointToY(
    zVec3 *endpoint,
    const zVec3 *otherEndpoint,
    float clipY
);
} // namespace HudLineClip
