#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetGameSetup.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstring>
#include <new>

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
        patch.address = 0;
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
    if (patch.address == 0) {
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
    patch.address = 0;
}

const char *g_netSetupLoadPath;
const char *g_netSetupLoadSection;
int g_netSetupLoadCapture;
zReader::Node *g_netSetupLoadResult;
int g_netSetupPrimitiveBindCount;
const char *g_netSetupPrimitiveBindNames[4];
HudUiElement *g_netSetupPrimitiveBindElements[4];
int g_netSetupWidgetBindCount;
const char *g_netSetupWidgetBindNames[24];
HudUiZrdWidget *g_netSetupWidgetBindWidgets[24];
int g_netSetupFreeCount;
int g_netSetupFreeArg;
int g_netSetupChildFlags;
char g_netSetupPlayerName[32];
int g_netSetupNetworkModem;
int g_netSetupZrdActivateCalls;
int g_netSetupCreateSessionCalls;
int g_netSetupCreateSessionResult;
zNetworkSessionDescStatusFields g_netSetupCapturedStatusFields;
int g_netSetupSetNetworkEnabledCalls;
int g_netSetupSetNetworkEnabledValue;
int g_netSetupCreateLocalPlayerCalls;
char *g_netSetupCreateLocalPlayerName;
int g_netSetupSetStatusFlagsCalls;
int g_netSetupSetStatusFlagsValue;
int g_netSetupTimerCalls;
int g_netSetupTimerRaw;
int g_netSetupTimerGoal;
int g_netSetupInitMissionCalls;
int g_netSetupInitMissionId;
int g_netSetupInitMissionFlags;
int g_netSetupQueueExitCalls;
int g_netSetupQueueExitParam;
int g_netSetupQueueSwitchCalls;
RecoilApp_IState *g_netSetupQueueSwitchState;
int g_netSetupQueueSwitchParam;
int g_netSetupDestructorCount;
void *g_netSetupDestructorThis[24];
int g_netSetupDestructorKind[24];
int g_netSetupSetIndexCalls;
int g_netSetupSetIndexArg;
int g_netSetupSetVisibleCount;
void *g_netSetupSetVisibleThis[4];
int g_netSetupSetVisibleValue[4];
int g_netSetupCommitCalls;
int g_netSetupCommitValue;
int g_netSetupUpdateCalls;
char g_netSetupUpdateText[24];
int g_netSetupRefreshCount;
void *g_netSetupRefreshThis[8];
int g_netSetupInvalidateCount;
void *g_netSetupInvalidateThis[8];
int g_netSetupSendPkt14Calls;
int g_netSetupSendPkt14EventCode;
unsigned int g_netSetupSendPkt14StatusFlags;
int g_netSetupSendPkt14ValueOrTime;
int g_netSetupSendPkt14AuxParam;
int g_netSetupIsHostValue;
int g_netSetupHostUpdateCalls;
int g_netSetupHostUpdateEventCode;
int g_netSetupHostUpdateAuxParam;
int g_netSetupHostUpdateValueOrTime;
int g_netSetupHostUpdateStatusFlags;
int g_netSetupUnregisterCalls;
int g_netSetupResetRemoteCalls;
alignas(CZRecoilFrame) unsigned char g_netSetupFrameStorage[sizeof(CZRecoilFrame)];
CZRecoilFrame &g_netSetupFrame =
    *reinterpret_cast<CZRecoilFrame *>(g_netSetupFrameStorage);
int g_netSetupTryHalfResCalls;
int g_netSetupTryHalfResMode;
int g_netSetupTryInvalidateCalls;
int g_netSetupTryInvalidateMode;
int g_netSetupTryPitchCalls;
int g_netSetupTryBppCalls;
int g_netSetupTryWindowCalls;
int g_netSetupTryPixelsCalls;
void *g_netSetupTryPixels;
zOpt_ViewRectSection *g_netSetupTryWindow;
void *g_netSetupTryFramePixels;
zOpt_ViewRectSection *g_netSetupTryFrameRect;
int g_netSetupTryFrameBpp;
int g_netSetupTryFramePitch;
int g_netSetupTrySampleCalls;
const char *g_netSetupTrySampleName;
int g_netSetupTryCdAudioOption;
int g_netSetupTryCdOptionCalls;
int g_netSetupTryCdPlayCalls;
int g_netSetupTryCdTrack;
int g_netSetupTryCdMode;

void ResetNetGameSetupProbe() {
    g_netSetupLoadPath = 0;
    g_netSetupLoadSection = 0;
    g_netSetupLoadCapture = -1;
    g_netSetupLoadResult = 0;
    g_netSetupPrimitiveBindCount = 0;
    for (int i = 0; i < 4; ++i) {
        g_netSetupPrimitiveBindNames[i] = 0;
        g_netSetupPrimitiveBindElements[i] = 0;
        g_netSetupSetVisibleThis[i] = 0;
        g_netSetupSetVisibleValue[i] = -1;
    }
    g_netSetupWidgetBindCount = 0;
    for (int j = 0; j < 24; ++j) {
        g_netSetupWidgetBindNames[j] = 0;
        g_netSetupWidgetBindWidgets[j] = 0;
        g_netSetupDestructorThis[j] = 0;
        g_netSetupDestructorKind[j] = 0;
    }
    g_netSetupFreeCount = 0;
    g_netSetupFreeArg = 0;
    g_netSetupChildFlags = -1;
    std::strcpy(
        g_netSetupPlayerName,
        "Pilot"
    );
    g_netSetupNetworkModem = 0;
    g_netSetupZrdActivateCalls = 0;
    g_netSetupCreateSessionCalls = 0;
    g_netSetupCreateSessionResult = 1;
    std::memset(
        &g_netSetupCapturedStatusFields,
        0,
        sizeof(g_netSetupCapturedStatusFields)
    );
    g_netSetupSetNetworkEnabledCalls = 0;
    g_netSetupSetNetworkEnabledValue = 0;
    g_netSetupCreateLocalPlayerCalls = 0;
    g_netSetupCreateLocalPlayerName = 0;
    g_netSetupSetStatusFlagsCalls = 0;
    g_netSetupSetStatusFlagsValue = 0;
    g_netSetupTimerCalls = 0;
    g_netSetupTimerRaw = 0;
    g_netSetupTimerGoal = 0;
    g_netSetupInitMissionCalls = 0;
    g_netSetupInitMissionId = 0;
    g_netSetupInitMissionFlags = 0;
    g_netSetupQueueExitCalls = 0;
    g_netSetupQueueExitParam = -1;
    g_netSetupQueueSwitchCalls = 0;
    g_netSetupQueueSwitchState = 0;
    g_netSetupQueueSwitchParam = -1;
    g_netSetupDestructorCount = 0;
    g_netSetupSetIndexCalls = 0;
    g_netSetupSetIndexArg = -1;
    g_netSetupSetVisibleCount = 0;
    g_netSetupCommitCalls = 0;
    g_netSetupCommitValue = 0;
    g_netSetupUpdateCalls = 0;
    g_netSetupUpdateText[0] = '\0';
    g_netSetupRefreshCount = 0;
    for (int refreshIndex = 0; refreshIndex < 8; ++refreshIndex) {
        g_netSetupRefreshThis[refreshIndex] = 0;
        g_netSetupInvalidateThis[refreshIndex] = 0;
    }
    g_netSetupInvalidateCount = 0;
    g_netSetupSendPkt14Calls = 0;
    g_netSetupSendPkt14EventCode = 0;
    g_netSetupSendPkt14StatusFlags = 0;
    g_netSetupSendPkt14ValueOrTime = 0;
    g_netSetupSendPkt14AuxParam = 0;
    g_netSetupIsHostValue = 0;
    g_netSetupHostUpdateCalls = 0;
    g_netSetupHostUpdateEventCode = 0;
    g_netSetupHostUpdateAuxParam = 0;
    g_netSetupHostUpdateValueOrTime = 0;
    g_netSetupHostUpdateStatusFlags = 0;
    g_netSetupUnregisterCalls = 0;
    g_netSetupResetRemoteCalls = 0;
    g_netSetupFrame.m_useArchiveBanks = 7;
}

void ResetNetGameSetupTryProbe() {
    g_netSetupTryHalfResCalls = 0;
    g_netSetupTryHalfResMode = -1;
    g_netSetupTryInvalidateCalls = 0;
    g_netSetupTryInvalidateMode = -1;
    g_netSetupTryPitchCalls = 0;
    g_netSetupTryBppCalls = 0;
    g_netSetupTryWindowCalls = 0;
    g_netSetupTryPixelsCalls = 0;
    g_netSetupTryPixels = reinterpret_cast<void *>(0x12345678);
    g_netSetupTryWindow = reinterpret_cast<zOpt_ViewRectSection *>(0x87654321);
    g_netSetupTryFramePixels = 0;
    g_netSetupTryFrameRect = 0;
    g_netSetupTryFrameBpp = 0;
    g_netSetupTryFramePitch = 0;
    g_netSetupTrySampleCalls = 0;
    g_netSetupTrySampleName = 0;
    g_netSetupTryCdAudioOption = 1;
    g_netSetupTryCdOptionCalls = 0;
    g_netSetupTryCdPlayCalls = 0;
    g_netSetupTryCdTrack = 0;
    g_netSetupTryCdMode = 0;
}

char *FakeNetGameSetupGetPlayerName() {
    return g_netSetupPlayerName;
}

int FakeNetGameSetupGetNetworkModemEnabled() {
    return g_netSetupNetworkModem;
}

void __fastcall FakeNetGameSetupSetNetworkEnabled(int value) {
    ++g_netSetupSetNetworkEnabledCalls;
    g_netSetupSetNetworkEnabledValue = value;
}

int __fastcall FakeNetGameSetupCreateSession(
    zNetworkSessionDescStatusFields *statusFields
) {
    ++g_netSetupCreateSessionCalls;
    g_netSetupCapturedStatusFields = *statusFields;
    return g_netSetupCreateSessionResult;
}

int __fastcall FakeNetGameSetupCreateLocalPlayer(char *playerName) {
    ++g_netSetupCreateLocalPlayerCalls;
    g_netSetupCreateLocalPlayerName = playerName;
    return 0x1234;
}

void __fastcall FakeNetGameSetupSetStatusBits(unsigned int statusFlags) {
    ++g_netSetupSetStatusFlagsCalls;
    g_netSetupSetStatusFlagsValue = static_cast<int>(statusFlags);
}

int __fastcall FakeNetGameSetupSendPkt14(
    int eventCode,
    unsigned int statusFlags,
    int valueOrTime,
    int auxParam
) {
    ++g_netSetupSendPkt14Calls;
    g_netSetupSendPkt14EventCode = eventCode;
    g_netSetupSendPkt14StatusFlags = statusFlags;
    g_netSetupSendPkt14ValueOrTime = valueOrTime;
    g_netSetupSendPkt14AuxParam = auxParam;
    return 0;
}

int FakeNetGameSetupIsHost() {
    return g_netSetupIsHostValue;
}

int __fastcall FakeNetGameSetupHostUpdate(
    int eventCode,
    int auxParam,
    int valueOrTime,
    int statusFlags
) {
    ++g_netSetupHostUpdateCalls;
    g_netSetupHostUpdateEventCode = eventCode;
    g_netSetupHostUpdateAuxParam = auxParam;
    g_netSetupHostUpdateValueOrTime = valueOrTime;
    g_netSetupHostUpdateStatusFlags = statusFlags;
    return 1;
}

void FakeNetGameSetupUnregisterHandlers() {
    ++g_netSetupUnregisterCalls;
}

void FakeNetGameSetupResetRemotePlayers() {
    ++g_netSetupResetRemoteCalls;
}

enum NetGameSetupDestructorKind {
    kNetGameSetupDestroyWidget = 1,
    kNetGameSetupDestroyCheckToggle = 2,
    kNetGameSetupDestroyZrdWidget = 3,
    kNetGameSetupDestroyNumericTextInput = 4,
    kNetGameSetupDestroyCycleSelector = 5,
    kNetGameSetupDestroyBackground = 6
};

void RecordNetGameSetupDestructor(
    void *object,
    int kind
) {
    const int index = g_netSetupDestructorCount;
    if (index < 24) {
        g_netSetupDestructorThis[index] = object;
        g_netSetupDestructorKind[index] = kind;
    }
    ++g_netSetupDestructorCount;
}

struct NetGameSetupPatchOps {
    void ZrdOnActivate() {
        ++g_netSetupZrdActivateCalls;
    }

    zReader::Node * LoadFromZrd(
        const char *path,
        const char *section,
        int capturePrimary
    ) {
        g_netSetupLoadPath = path;
        g_netSetupLoadSection = section;
        g_netSetupLoadCapture = capturePrimary;
        return g_netSetupLoadResult;
    }

    int BindPrimitiveNodeToElement(
        zReader::Node *,
        HudUiElement *element,
        const char *name
    ) {
        const int index = g_netSetupPrimitiveBindCount;
        if (index < 4) {
            g_netSetupPrimitiveBindNames[index] = name;
            g_netSetupPrimitiveBindElements[index] = element;
        }
        ++g_netSetupPrimitiveBindCount;
        return 1;
    }

    int BindWidgetByName(
        zReader::Node *,
        HudUiZrdWidget *widget,
        const char *name
    ) {
        const int index = g_netSetupWidgetBindCount;
        if (index < 24) {
            g_netSetupWidgetBindNames[index] = name;
            g_netSetupWidgetBindWidgets[index] = widget;
        }
        ++g_netSetupWidgetBindCount;
        return 1;
    }

    void FreeLoadedTreeRoots(int rootArg) {
        ++g_netSetupFreeCount;
        g_netSetupFreeArg = rootArg;
    }

    void SetChildFlags(unsigned int flags) {
        g_netSetupChildFlags = static_cast<int>(flags);
    }

    void SetRuntimeTimerSecAndGoalValue(
        int timerSecRaw,
        int goalValue
    ) {
        ++g_netSetupTimerCalls;
        g_netSetupTimerRaw = timerSecRaw;
        g_netSetupTimerGoal = goalValue;
    }

    int InitMissionIdAndFlags(
        int missionId,
        int flags
    ) {
        ++g_netSetupInitMissionCalls;
        g_netSetupInitMissionId = missionId;
        g_netSetupInitMissionFlags = flags;
        return 1;
    }

    CZRecoilFrame * GetMainWnd() const {
        return &g_netSetupFrame;
    }

    RecoilApp_IState * QueueExitCurrentState(int stateParam) {
        ++g_netSetupQueueExitCalls;
        g_netSetupQueueExitParam = stateParam;
        return 0;
    }

    RecoilApp_IState * QueueSwitchCurrentState(
        RecoilApp_IState *state,
        int stateParam
    ) {
        ++g_netSetupQueueSwitchCalls;
        g_netSetupQueueSwitchState = state;
        g_netSetupQueueSwitchParam = stateParam;
        return 0;
    }

    int SetIndexClamped(int index) {
        ++g_netSetupSetIndexCalls;
        g_netSetupSetIndexArg = index;
        ((HudUiCycleSelectorWidget *)(this))->selectedIndex = index;
        return index;
    }

    void SetVisible(int visible) {
        const int index = g_netSetupSetVisibleCount;
        if (index < 4) {
            g_netSetupSetVisibleThis[index] = this;
            g_netSetupSetVisibleValue[index] = visible;
        }
        ++g_netSetupSetVisibleCount;
    }

    int CommitAndGetValue() {
        ++g_netSetupCommitCalls;
        return g_netSetupCommitValue;
    }

    void NumericTextInputUpdate(const char *text) {
        ++g_netSetupUpdateCalls;
        std::strncpy(
            g_netSetupUpdateText,
            text,
            sizeof(g_netSetupUpdateText) - 1
        );
        g_netSetupUpdateText[sizeof(g_netSetupUpdateText) - 1] = '\0';
    }

    void RefreshState() {
        const int index = g_netSetupRefreshCount;
        if (index < 8) {
            g_netSetupRefreshThis[index] = this;
        }
        ++g_netSetupRefreshCount;
    }

    void Invalidate() {
        const int index = g_netSetupInvalidateCount;
        if (index < 8) {
            g_netSetupInvalidateThis[index] = this;
        }
        ++g_netSetupInvalidateCount;
    }

    void WidgetDestructorCore() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyWidget
        );
    }

    void CheckToggleDestructorCore() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyCheckToggle
        );
    }

    void ZrdWidgetDestructorCore() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyZrdWidget
        );
    }

    void NumericTextInputDestructor() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyNumericTextInput
        );
    }

    void CycleSelectorDestructorCore() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyCycleSelector
        );
    }

    void BackgroundDestructor() {
        RecordNetGameSetupDestructor(
            this,
            kNetGameSetupDestroyBackground
        );
    }
};

void RestorePatches(
    CodeFunctionPatch *patches,
    int patchCount
) {
    while (patchCount > 0) {
        --patchCount;
        RestoreFunctionPatch(patches[patchCount]);
    }
}

HudUiNetGameSetupPanel *AllocZeroPanel() {
    void *const storage = ::operator new(sizeof(HudUiNetGameSetupPanel));
    std::memset(
        storage,
        0,
        sizeof(HudUiNetGameSetupPanel)
    );
    return static_cast<HudUiNetGameSetupPanel *>(storage);
}

void FreeZeroPanel(HudUiNetGameSetupPanel *panel) {
    ::operator delete(panel);
}

struct NetGameSetupTryPatchOps {
    static int __fastcall SetHalfResAdjustMode(int mode) {
        ++g_netSetupTryHalfResCalls;
        g_netSetupTryHalfResMode = mode;
        return 3;
    }

    static void __fastcall SetInvalidateMode(int mode) {
        ++g_netSetupTryInvalidateCalls;
        g_netSetupTryInvalidateMode = mode;
    }

    static int GetPrimarySurfacePitch() {
        ++g_netSetupTryPitchCalls;
        return 3200;
    }

    static int GetDisplaySectionBitsPerPixel() {
        ++g_netSetupTryBppCalls;
        return 16;
    }

    static zOpt_ViewRectSection *GetWindowSection() {
        ++g_netSetupTryWindowCalls;
        return g_netSetupTryWindow;
    }

    static void *GetPrimarySurfacePixels() {
        ++g_netSetupTryPixelsCalls;
        return g_netSetupTryPixels;
    }

    static void __fastcall SetFrameBufferRegion(
        void *pixels,
        zOpt_ViewRectSection *region,
        int bitsPerPixel,
        int pitchBytes
    ) {
        g_netSetupTryFramePixels = pixels;
        g_netSetupTryFrameRect = region;
        g_netSetupTryFrameBpp = bitsPerPixel;
        g_netSetupTryFramePitch = pitchBytes;
    }

    static int __fastcall InitSampleSetByName(const char *setName) {
        ++g_netSetupTrySampleCalls;
        g_netSetupTrySampleName = setName;
        return 1;
    }

    static int GetCDAudioOption() {
        ++g_netSetupTryCdOptionCalls;
        return g_netSetupTryCdAudioOption;
    }

    static int __fastcall PlayTrackWithMode(int track, int mode) {
        ++g_netSetupTryCdPlayCalls;
        g_netSetupTryCdTrack = track;
        g_netSetupTryCdMode = mode;
        return 1;
    }
};

bool InstallConstructorPatches(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    bool installed = true;
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiBackground::LoadFromZrd),
            MethodAddress(&NetGameSetupPatchOps::LoadFromZrd),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiBackground::BindPrimitiveNodeToElement),
            MethodAddress(&NetGameSetupPatchOps::BindPrimitiveNodeToElement),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiBackground::BindWidgetByName),
            MethodAddress(&NetGameSetupPatchOps::BindWidgetByName),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiBackground::FreeLoadedTreeRoots),
            MethodAddress(&NetGameSetupPatchOps::FreeLoadedTreeRoots),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiContainer::SetChildFlags),
            MethodAddress(&NetGameSetupPatchOps::SetChildFlags),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zOpt_GetPlayerName),
            (void *)(&FakeNetGameSetupGetPlayerName),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zOpt::GetNetworkModemEnabled),
            (void *)(&FakeNetGameSetupGetNetworkModemEnabled),
            patches[patchCount++]
        );
    return installed;
}

bool InstallOverlayTryPatches(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zVideo::SetHalfResAdjustMode),
            (void *)(&NetGameSetupTryPatchOps::SetHalfResAdjustMode),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&HudUi::SetInvalidateMode),
            (void *)(&NetGameSetupTryPatchOps::SetInvalidateMode),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zVideo::GetPrimarySurfacePitch),
            (void *)(&NetGameSetupTryPatchOps::GetPrimarySurfacePitch),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zOpt::GetDisplaySectionBitsPerPixel),
            (void *)(&NetGameSetupTryPatchOps::GetDisplaySectionBitsPerPixel),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zOpt::GetWindowSection),
            (void *)(&NetGameSetupTryPatchOps::GetWindowSection),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zVideo::GetPrimarySurfacePixels),
            (void *)(&NetGameSetupTryPatchOps::GetPrimarySurfacePixels),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zRndr::SetFrameBufferRegion),
            (void *)(&NetGameSetupTryPatchOps::SetFrameBufferRegion),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zSndSampleSet_InitByName),
            (void *)(&NetGameSetupTryPatchOps::InitSampleSetByName),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zSnd::GetCDAudioOption),
            (void *)(&NetGameSetupTryPatchOps::GetCDAudioOption),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zSndCd::PlayTrackWithMode),
            (void *)(&NetGameSetupTryPatchOps::PlayTrackWithMode),
            patches[patchCount++]
        );
    return installed;
}
} // namespace

extern "C" int hud_ui_net_game_setup_overlay_owner_on_try_smoke(void) {
    CodeFunctionPatch patches[18] = {};
    int patchCount = 0;
    const bool installed = InstallOverlayTryPatches(
        patches,
        patchCount
    );

    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    ResetNetGameSetupTryProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    g_netSetupTryCdAudioOption = 1;

    HudUiNetGameSetupOverlayOwner state;
    state.m_reconfigureExistingSession = 1;
    const int result = installed ? state.OnTryBecomeCurrent() : 0;
    HudUiNetGameSetupPanel *const panel =
        static_cast<HudUiNetGameSetupPanel *>(state.m_dialog);
    const bool cdEnabledPath =
        installed &&
        result == 1 &&
        panel != 0 &&
        panel->reconfigureExistingSession == 1 &&
        panel->enabled == 1 &&
        g_netSetupTryHalfResCalls == 1 &&
        g_netSetupTryHalfResMode == 0 &&
        g_netSetupTryInvalidateCalls == 1 &&
        g_netSetupTryInvalidateMode == 0 &&
        g_netSetupTryPitchCalls == 1 &&
        g_netSetupTryBppCalls == 1 &&
        g_netSetupTryWindowCalls == 1 &&
        g_netSetupTryPixelsCalls == 1 &&
        g_netSetupTryFramePixels == g_netSetupTryPixels &&
        g_netSetupTryFrameRect == g_netSetupTryWindow &&
        g_netSetupTryFrameBpp == 16 &&
        g_netSetupTryFramePitch == 3200 &&
        g_netSetupTrySampleCalls == 1 &&
        std::strcmp(g_netSetupTrySampleName, "DIALOG") == 0 &&
        std::strcmp(g_netSetupLoadPath, "dialog.zrd") == 0 &&
        std::strcmp(g_netSetupLoadSection, "MP_NEW_GAME") == 0 &&
        g_netSetupTryCdOptionCalls == 1 &&
        g_netSetupTryCdPlayCalls == 1 &&
        g_netSetupTryCdTrack == 2 &&
        g_netSetupTryCdMode == 5;
    if (panel != 0) {
        panel->Destructor();
        ::operator delete(panel);
    }

    state.m_dialog = 0;
    state.m_reconfigureExistingSession = 0;
    ResetNetGameSetupProbe();
    ResetNetGameSetupTryProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    g_netSetupTryCdAudioOption = 0;
    const int noCdResult = installed ? state.OnTryBecomeCurrent() : 0;
    HudUiNetGameSetupPanel *const noCdPanel =
        static_cast<HudUiNetGameSetupPanel *>(state.m_dialog);
    const bool cdDisabledPath =
        installed &&
        noCdResult == 1 &&
        noCdPanel != 0 &&
        noCdPanel->reconfigureExistingSession == 0 &&
        noCdPanel->enabled == 1 &&
        g_netSetupTryCdOptionCalls == 1 &&
        g_netSetupTryCdPlayCalls == 0;
    if (noCdPanel != 0) {
        noCdPanel->Destructor();
        ::operator delete(noCdPanel);
    }
    state.m_dialog = 0;

    RestorePatches(
        patches,
        patchCount
    );
    if (!cdEnabledPath) {
        return 2;
    }
    if (!cdDisabledPath) {
        return 3;
    }
    return 0;
}

extern "C" int hud_ui_net_game_setup_panel_constructor_smoke(void) {
    CodeFunctionPatch patches[8] = {};
    int patchCount = 0;
    const bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );

    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    std::strcpy(
        g_netSetupPlayerName,
        "PilotOne"
    );
    g_netSetupNetworkModem = 0;

    HudUiNetGameSetupPanel *const panel = AllocZeroPanel();
    const bool returnedSelf = installed && panel->Constructor(0) == panel;
    const bool commonInit =
        returnedSelf &&
        panel->currentFocusWidget == 0 &&
        panel->reconfigureExistingSession == 0 &&
        std::strcmp(g_netSetupLoadPath, "dialog.zrd") == 0 &&
        std::strcmp(g_netSetupLoadSection, "MP_NEW_GAME") == 0 &&
        g_netSetupLoadCapture == 0 &&
        g_netSetupPrimitiveBindCount == 2 &&
        std::strcmp(g_netSetupPrimitiveBindNames[0], "KILLS_SWITCH") == 0 &&
        g_netSetupPrimitiveBindElements[0] == (HudUiElement *)(&panel->killsSwitch) &&
        std::strcmp(g_netSetupPrimitiveBindNames[1], "LAPS_SWITCH") == 0 &&
        g_netSetupPrimitiveBindElements[1] == (HudUiElement *)(&panel->lapsSwitch) &&
        g_netSetupWidgetBindCount == 17 &&
        std::strcmp(g_netSetupWidgetBindNames[0], "PLAY") == 0 &&
        g_netSetupWidgetBindWidgets[0] == &panel->playButton &&
        std::strcmp(g_netSetupWidgetBindNames[16], "NAME_TAGS") == 0 &&
        g_netSetupWidgetBindWidgets[16] == &panel->nameTagsToggle &&
        g_netSetupFreeCount == 1 &&
        g_netSetupChildFlags == 0 &&
        panel->gameNameInput.modeOrEnabled == 1 &&
        panel->timeLimitInput.minValue == 5 &&
        panel->timeLimitInput.maxValue == 360 &&
        panel->incTimeLimitButton.targetInput == &panel->timeLimitInput &&
        panel->decTimeLimitButton.stepDelta == -1 &&
        panel->killsInput.minValue == 1 &&
        panel->killsInput.maxValue == 99 &&
        panel->maxPlayersInput.minValue == 2 &&
        panel->maxPlayersInput.maxValue == 8 &&
        panel->maxPlayersInput.modeOrEnabled == 1 &&
        panel->allowMapsToggle.checked == 1 &&
        panel->nameTagsToggle.checked == 0;
    panel->Destructor();
    FreeZeroPanel(panel);

    ResetNetGameSetupProbe();
    g_netSetupNetworkModem = 1;
    HudUiNetGameSetupPanel *const reconfigurePanel = AllocZeroPanel();
    const bool reconfigureInit =
        installed &&
        reconfigurePanel->Constructor(1) == reconfigurePanel &&
        reconfigurePanel->reconfigureExistingSession == 1 &&
        reconfigurePanel->gameNameInput.modeOrEnabled == 0 &&
        reconfigurePanel->maxPlayersInput.modeOrEnabled == 0 &&
        reconfigurePanel->incMaxPlayersButton.modeOrEnabled == 0 &&
        reconfigurePanel->decMaxPlayersButton.modeOrEnabled == 0 &&
        g_netSetupPrimitiveBindCount == 0 &&
        g_netSetupWidgetBindCount == 0 &&
        g_netSetupFreeCount == 0;
    reconfigurePanel->Destructor();
    FreeZeroPanel(reconfigurePanel);

    RestorePatches(
        patches,
        patchCount
    );
    return commonInit && reconfigureInit ? 0 : 1;
}

extern "C" int hud_ui_net_game_setup_cancel_button_smoke(void) {
    CodeFunctionPatch patches[3] = {};
    int patchCount = 0;
    bool installed = true;
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            MethodAddress(&NetGameSetupPatchOps::ZrdOnActivate),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&RecoilApp::QueueExitCurrentState),
            MethodAddress(&NetGameSetupPatchOps::QueueExitCurrentState),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&RecoilApp::QueueSwitchCurrentState),
            MethodAddress(&NetGameSetupPatchOps::QueueSwitchCurrentState),
            patches[patchCount++]
        );

    ResetNetGameSetupProbe();
    HudUiNetGameSetupPanel_CancelButton button{};
    button.Constructor();
    if (installed) {
        button.OnActivate();
    }

    RestorePatches(
        patches,
        patchCount
    );
    if (!installed) {
        return 10;
    }
    if (g_netSetupQueueExitCalls != 1 || g_netSetupQueueExitParam != 0) {
        return 2;
    }
    if (g_netSetupQueueSwitchCalls != 1 || g_netSetupQueueSwitchParam != 0) {
        return 3;
    }
    if (g_netSetupQueueSwitchState != (RecoilApp_IState *)(&g_RecoilApp.m_leaveNetworkState)) {
        return 4;
    }
    return 0;
}

extern "C" int hud_ui_net_game_setup_panel_destructor_smoke(void) {
    CodeFunctionPatch patches[8] = {};
    int patchCount = 0;
    const bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );
    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    HudUiNetGameSetupPanel *const panel = AllocZeroPanel();
    const bool constructed = installed && panel->Constructor(0) == panel;
    if (constructed) {
        panel->Destructor();
    }
    FreeZeroPanel(panel);

    RestorePatches(
        patches,
        patchCount
    );
    return constructed ? 0 : 1;
}

bool InstallWorldButtonPatches(
    CodeFunctionPatch *patches,
    int &patchCount
) {
    bool installed = true;
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiCycleSelectorWidget::SetIndexClamped),
            MethodAddress(&NetGameSetupPatchOps::SetIndexClamped),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::RefreshState),
            MethodAddress(&NetGameSetupPatchOps::RefreshState),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::Invalidate),
            MethodAddress(&NetGameSetupPatchOps::Invalidate),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudUiZrdWidget::OnActivate),
            MethodAddress(&NetGameSetupPatchOps::ZrdOnActivate),
            patches[patchCount++]
        );
    return installed;
}

bool VerifyWorldButtonEffects(
    HudUiNetGameSetupPanel *panel,
    int setIndexArg,
    int setVisible0,
    int timeEnabled
) {
    return
        g_netSetupSetIndexCalls == 1 &&
        g_netSetupSetIndexArg == setIndexArg &&
        ((panel->killsSwitch.flags & 0x10u) ==
            (setVisible0 == 0 ? 0x10u : 0u)) &&
        ((panel->lapsSwitch.flags & 0x10u) ==
            (setVisible0 == 0 ? 0u : 0x10u)) &&
        panel->timeLimitInput.modeOrEnabled == 1 &&
        panel->incTimeLimitButton.modeOrEnabled == timeEnabled &&
        panel->decTimeLimitButton.modeOrEnabled == timeEnabled;
}

int VerifyWorldButtonEffectsFailure(
    HudUiNetGameSetupPanel *panel,
    int setIndexArg,
    int setVisible0,
    int timeEnabled
) {
    if (g_netSetupSetIndexCalls != 1) {
        return 1;
    }
    if (g_netSetupSetIndexArg != setIndexArg) {
        return 2;
    }
    if ((panel->killsSwitch.flags & 0x10u) !=
        (setVisible0 == 0 ? 0x10u : 0u)) {
        return 3;
    }
    if ((panel->lapsSwitch.flags & 0x10u) !=
        (setVisible0 == 0 ? 0u : 0x10u)) {
        return 4;
    }
    if (panel->timeLimitInput.modeOrEnabled != 1) {
        return 5;
    }
    if (panel->incTimeLimitButton.modeOrEnabled != timeEnabled) {
        return 6;
    }
    if (panel->decTimeLimitButton.modeOrEnabled != timeEnabled) {
        return 7;
    }
    return 0;
}

extern "C" int hud_ui_net_game_setup_next_world_button_smoke(void) {
    CodeFunctionPatch patches[16] = {};
    int patchCount = 0;
    bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );
    installed = installed && InstallWorldButtonPatches(
        patches,
        patchCount
    );

    HudUiNetGameSetupPanel *const panel = AllocZeroPanel();
    HudUiNetGameSetupPanel_NextWorldButton button{};
    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    const bool constructed = installed && panel->Constructor(0) == panel;
    button.Constructor();
    button.owner = panel;

    ResetNetGameSetupProbe();
    g_netSetupCommitValue = 1;
    panel->worldSelector.selectedIndex = 1;
    panel->killsInput.Update("1");
    if (installed) {
        button.OnActivate();
    }
    int lapsPathCode = 0;
    if (!constructed) {
        lapsPathCode = 2;
    } else if (panel->worldSelector.selectedIndex != 2) {
        lapsPathCode = 3;
    } else if (panel->killsInput.minValue != 2 || panel->killsInput.maxValue != 99) {
        lapsPathCode = 4;
    } else if (std::strcmp(panel->killsInput.GetBuffer(), "2") != 0) {
        lapsPathCode = 6;
    } else {
        const int effectsFailure = VerifyWorldButtonEffectsFailure(
            panel,
            2,
            0,
            0
        );
        if (effectsFailure != 0) {
            lapsPathCode = 70 + effectsFailure;
        }
    }
    const bool lapsPath =
        constructed &&
        panel->worldSelector.selectedIndex == 2 &&
        panel->killsInput.minValue == 2 &&
        panel->killsInput.maxValue == 99 &&
        std::strcmp(panel->killsInput.GetBuffer(), "2") == 0 &&
        VerifyWorldButtonEffects(
            panel,
            2,
            0,
            0
        );

    ResetNetGameSetupProbe();
    panel->worldSelector.selectedIndex = 2;
    panel->timeLimitInput.modeOrEnabled = 0;
    panel->incTimeLimitButton.modeOrEnabled = 0;
    panel->decTimeLimitButton.modeOrEnabled = 0;
    if (installed) {
        button.OnActivate();
    }
    int killsPathCode = 0;
    if (!constructed) {
        killsPathCode = 8;
    } else if (panel->worldSelector.selectedIndex != 3) {
        killsPathCode = 9;
    } else if (panel->killsInput.minValue != 1 || panel->killsInput.maxValue != 99) {
        killsPathCode = 10;
    } else {
        const int effectsFailure = VerifyWorldButtonEffectsFailure(
            panel,
            3,
            1,
            1
        );
        if (effectsFailure != 0) {
            killsPathCode = 80 + effectsFailure;
        }
    }
    const bool killsPath =
        constructed &&
        panel->worldSelector.selectedIndex == 3 &&
        panel->killsInput.minValue == 1 &&
        panel->killsInput.maxValue == 99 &&
        VerifyWorldButtonEffects(
            panel,
            3,
            1,
            1
        );

    RestorePatches(
        patches,
        patchCount
    );
    panel->Destructor();
    FreeZeroPanel(panel);
    if (lapsPathCode != 0) {
        return lapsPathCode;
    }
    if (killsPathCode != 0) {
        return killsPathCode;
    }
    return lapsPath && killsPath ? 0 : 1;
}

extern "C" int hud_ui_net_game_setup_prev_world_button_smoke(void) {
    CodeFunctionPatch patches[16] = {};
    int patchCount = 0;
    bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );
    installed = installed && InstallWorldButtonPatches(
        patches,
        patchCount
    );

    HudUiNetGameSetupPanel *const panel = AllocZeroPanel();
    HudUiNetGameSetupPanel_PrevWorldButton button{};
    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    const bool constructed = installed && panel->Constructor(0) == panel;
    button.Constructor();
    button.owner = panel;

    ResetNetGameSetupProbe();
    g_netSetupCommitValue = 1;
    panel->worldSelector.selectedIndex = 3;
    panel->killsInput.Update("1");
    if (installed) {
        button.OnActivate();
    }
    int lapsPathCode = 0;
    if (!constructed) {
        lapsPathCode = 22;
    } else if (panel->worldSelector.selectedIndex != 2) {
        lapsPathCode = 23;
    } else if (panel->killsInput.minValue != 2 || panel->killsInput.maxValue != 99) {
        lapsPathCode = 24;
    } else if (std::strcmp(panel->killsInput.GetBuffer(), "2") != 0) {
        lapsPathCode = 26;
    } else {
        const int effectsFailure = VerifyWorldButtonEffectsFailure(
            panel,
            2,
            0,
            0
        );
        if (effectsFailure != 0) {
            lapsPathCode = 90 + effectsFailure;
        }
    }
    const bool lapsPath =
        constructed &&
        panel->worldSelector.selectedIndex == 2 &&
        panel->killsInput.minValue == 2 &&
        panel->killsInput.maxValue == 99 &&
        std::strcmp(panel->killsInput.GetBuffer(), "2") == 0 &&
        VerifyWorldButtonEffects(
            panel,
            2,
            0,
            0
        );

    ResetNetGameSetupProbe();
    panel->worldSelector.selectedIndex = 2;
    panel->timeLimitInput.modeOrEnabled = 0;
    panel->incTimeLimitButton.modeOrEnabled = 0;
    panel->decTimeLimitButton.modeOrEnabled = 0;
    if (installed) {
        button.OnActivate();
    }
    int killsPathCode = 0;
    if (!constructed) {
        killsPathCode = 28;
    } else if (panel->worldSelector.selectedIndex != 1) {
        killsPathCode = 29;
    } else if (panel->killsInput.minValue != 1 || panel->killsInput.maxValue != 99) {
        killsPathCode = 30;
    } else {
        const int effectsFailure = VerifyWorldButtonEffectsFailure(
            panel,
            1,
            1,
            1
        );
        if (effectsFailure != 0) {
            killsPathCode = 100 + effectsFailure;
        }
    }
    const bool killsPath =
        constructed &&
        panel->worldSelector.selectedIndex == 1 &&
        panel->killsInput.minValue == 1 &&
        panel->killsInput.maxValue == 99 &&
        VerifyWorldButtonEffects(
            panel,
            1,
            1,
            1
        );

    RestorePatches(
        patches,
        patchCount
    );
    panel->Destructor();
    FreeZeroPanel(panel);
    if (lapsPathCode != 0) {
        return lapsPathCode;
    }
    if (killsPathCode != 0) {
        return killsPathCode;
    }
    return lapsPath && killsPath ? 0 : 1;
}

extern "C" int hud_ui_net_game_setup_launch_button_smoke(void) {
    CodeFunctionPatch patches[24] = {};
    int patchCount = 0;
    bool installed = InstallConstructorPatches(
        patches,
        patchCount
    );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zOpt::SetNetworkEnabled),
            (void *)(&FakeNetGameSetupSetNetworkEnabled),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zNetwork_DPlay::CreateSessionFromStatusFields),
            (void *)(&FakeNetGameSetupCreateSession),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zNetwork_DPlay::CreateLocalPlayerRecordAndRegister),
            (void *)(&FakeNetGameSetupCreateLocalPlayer),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&GameNet::SetStatusBitsFromFlags),
            (void *)(&FakeNetGameSetupSetStatusBits),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&GameNet::SendPkt14_HudTimerAndFlagsSync),
            (void *)(&FakeNetGameSetupSendPkt14),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&zNetwork::IsHost),
            (void *)(&FakeNetGameSetupIsHost),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&GameNet::HostUpdateSessionDescStatusFields),
            (void *)(&FakeNetGameSetupHostUpdate),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&GameNet::UnregisterGameplayPacketHandlers),
            (void *)(&FakeNetGameSetupUnregisterHandlers),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            (void *)(&GameNet::ResetRemotePlayersAndSpawnLists),
            (void *)(&FakeNetGameSetupResetRemotePlayers),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudSensorTracker::SetRuntimeTimerSecAndGoalValue),
            MethodAddress(&NetGameSetupPatchOps::SetRuntimeTimerSecAndGoalValue),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&HudSensorTracker::InitMissionIdAndFlags),
            MethodAddress(&NetGameSetupPatchOps::InitMissionIdAndFlags),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&RecoilApp::GetMainWnd),
            MethodAddress(&NetGameSetupPatchOps::GetMainWnd),
            patches[patchCount++]
        );
    installed = installed &&
        PatchFunctionJump(
            MethodAddress(&RecoilApp::QueueExitCurrentState),
            MethodAddress(&NetGameSetupPatchOps::QueueExitCurrentState),
            patches[patchCount++]
        );

    char fakeNodeStorage = 0;
    ResetNetGameSetupProbe();
    g_netSetupLoadResult = reinterpret_cast<zReader::Node *>(&fakeNodeStorage);
    std::strcpy(
        g_netSetupPlayerName,
        "HostPilot"
    );

    HudUiNetGameSetupPanel *const panel = AllocZeroPanel();
    const bool constructed = installed && panel->Constructor(0) == panel;
    panel->playButton.owner = panel;
    panel->allowMapsToggle.checked = 1;
    panel->nameTagsToggle.checked = 1;
    panel->worldSelector.selectedIndex = 2;
    panel->gameNameInput.Update("Arena");
    panel->timeLimitInput.Update("15");
    panel->killsInput.Update("10");
    panel->maxPlayersInput.Update("6");
    g_RecoilApp.m_skipIntroFmv = 0;
    if (installed) {
        panel->playButton.HudUiNetGameSetupPanel_LaunchButton::OnActivate();
    }

    union ExpectedTimerRaw {
        float seconds;
        int raw;
    } expectedTimer = {900.0f};
    const bool newSessionOk =
        constructed &&
        g_netSetupCreateSessionCalls == 1 &&
        g_netSetupCapturedStatusFields.eventCode == 3 &&
        g_netSetupCapturedStatusFields.statusFlags == 3 &&
        g_netSetupCapturedStatusFields.valueOrTime == 15 &&
        g_netSetupCapturedStatusFields.auxParam == 10 &&
        g_netSetupCapturedStatusFields.maxPlayers == 6 &&
        std::strcmp(g_netSetupCapturedStatusFields.sessionNameBuf, "Arena") == 0 &&
        g_netSetupSetNetworkEnabledCalls == 1 &&
        g_netSetupSetNetworkEnabledValue == 1 &&
        g_netSetupCreateLocalPlayerCalls == 1 &&
        g_netSetupCreateLocalPlayerName == g_netSetupPlayerName &&
        g_netSetupSendPkt14Calls == 0 &&
        g_netSetupUnregisterCalls == 0 &&
        g_netSetupResetRemoteCalls == 0 &&
        g_netSetupSetStatusFlagsValue == 3 &&
        g_netSetupTimerRaw == expectedTimer.raw &&
        g_netSetupTimerGoal == 10 &&
        g_netSetupInitMissionId == 9 &&
        g_netSetupInitMissionFlags == 7 &&
        g_netSetupQueueExitCalls == 1 &&
        g_netSetupQueueExitParam == 0 &&
        g_RecoilApp.m_skipIntroFmv == 1;
    int newSessionCode = 0;
    if (!constructed) {
        newSessionCode = 2;
    } else if (g_netSetupCreateSessionCalls != 1) {
        if (g_netSetupSendPkt14Calls == 1) {
            newSessionCode = 19;
        } else if (g_netSetupSetStatusFlagsCalls == 1 ||
                   g_netSetupTimerCalls == 1 ||
                   g_netSetupQueueExitCalls == 1) {
            newSessionCode = 20;
        } else if (g_RecoilApp.m_skipIntroFmv == 1) {
            newSessionCode = 21;
        } else {
            newSessionCode = 3;
        }
    } else if (g_netSetupCapturedStatusFields.eventCode != 3) {
        newSessionCode = 4;
    } else if (g_netSetupCapturedStatusFields.statusFlags != 3) {
        newSessionCode = 5;
    } else if (g_netSetupCapturedStatusFields.valueOrTime != 15) {
        newSessionCode = 6;
    } else if (g_netSetupCapturedStatusFields.auxParam != 10) {
        newSessionCode = 7;
    } else if (g_netSetupCapturedStatusFields.maxPlayers != 6) {
        newSessionCode = 8;
    } else if (std::strcmp(g_netSetupCapturedStatusFields.sessionNameBuf, "Arena") != 0) {
        newSessionCode = 9;
    } else if (g_netSetupSetNetworkEnabledCalls != 1 ||
               g_netSetupSetNetworkEnabledValue != 1) {
        newSessionCode = 10;
    } else if (g_netSetupCreateLocalPlayerCalls != 1) {
        newSessionCode = 11;
    } else if (g_netSetupCreateLocalPlayerName != g_netSetupPlayerName) {
        newSessionCode = 12;
    } else if (g_netSetupSendPkt14Calls != 0 ||
               g_netSetupUnregisterCalls != 0 ||
               g_netSetupResetRemoteCalls != 0) {
        newSessionCode = 13;
    } else if (g_netSetupSetStatusFlagsValue != 3) {
        newSessionCode = 14;
    } else if (g_netSetupTimerRaw != expectedTimer.raw ||
               g_netSetupTimerGoal != 10) {
        newSessionCode = 15;
    } else if (g_netSetupInitMissionId != 9 ||
               g_netSetupInitMissionFlags != 7) {
        newSessionCode = 16;
    } else if (g_netSetupQueueExitCalls != 1 ||
               g_netSetupQueueExitParam != 0) {
        newSessionCode = 17;
    } else if (g_RecoilApp.m_skipIntroFmv != 1) {
        newSessionCode = 18;
    }

    ResetNetGameSetupProbe();
    g_netSetupNetworkModem = 1;
    g_netSetupIsHostValue = 1;
    g_netSetupFrame.m_useArchiveBanks = 11;
    g_RecoilApp.m_skipIntroFmv = 0;
    if (installed) {
        panel->playButton.HudUiNetGameSetupPanel_LaunchButton::OnActivate();
    }

    const bool reconfigureOk =
        g_netSetupCreateSessionCalls == 0 &&
        g_netSetupCreateLocalPlayerCalls == 0 &&
        g_netSetupSendPkt14Calls == 1 &&
        g_netSetupSendPkt14EventCode == 3 &&
        g_netSetupSendPkt14StatusFlags == 3 &&
        g_netSetupSendPkt14ValueOrTime == 15 &&
        g_netSetupSendPkt14AuxParam == 10 &&
        g_netSetupHostUpdateCalls == 1 &&
        g_netSetupHostUpdateEventCode == 3 &&
        g_netSetupHostUpdateAuxParam == 10 &&
        g_netSetupHostUpdateValueOrTime == 15 &&
        g_netSetupHostUpdateStatusFlags == 3 &&
        g_netSetupUnregisterCalls == 1 &&
        g_netSetupResetRemoteCalls == 1 &&
        g_netSetupSetStatusFlagsValue == 3 &&
        g_netSetupTimerRaw == expectedTimer.raw &&
        g_netSetupTimerGoal == 10 &&
        g_netSetupInitMissionId == 9 &&
        g_netSetupInitMissionFlags == 11 &&
        g_netSetupQueueExitCalls == 1 &&
        g_netSetupQueueExitParam == 0 &&
        g_RecoilApp.m_skipIntroFmv == 1;
    int reconfigureCode = 0;
    if (g_netSetupCreateSessionCalls != 0 ||
        g_netSetupCreateLocalPlayerCalls != 0) {
        reconfigureCode = 40;
    } else if (g_netSetupSendPkt14Calls != 1) {
        reconfigureCode = 41;
    } else if (g_netSetupSendPkt14EventCode != 3 ||
               g_netSetupSendPkt14StatusFlags != 3 ||
               g_netSetupSendPkt14ValueOrTime != 15 ||
               g_netSetupSendPkt14AuxParam != 10) {
        reconfigureCode = 42;
    } else if (g_netSetupHostUpdateCalls != 1) {
        reconfigureCode = 43;
    } else if (g_netSetupHostUpdateEventCode != 3 ||
               g_netSetupHostUpdateAuxParam != 10 ||
               g_netSetupHostUpdateValueOrTime != 15 ||
               g_netSetupHostUpdateStatusFlags != 3) {
        reconfigureCode = 44;
    } else if (g_netSetupUnregisterCalls != 1 ||
               g_netSetupResetRemoteCalls != 1) {
        reconfigureCode = 45;
    } else if (g_netSetupSetStatusFlagsValue != 3) {
        reconfigureCode = 46;
    } else if (g_netSetupTimerRaw != expectedTimer.raw ||
               g_netSetupTimerGoal != 10) {
        reconfigureCode = 47;
    } else if (g_netSetupInitMissionId != 9 ||
               g_netSetupInitMissionFlags != 11) {
        reconfigureCode = 48;
    } else if (g_netSetupQueueExitCalls != 1 ||
               g_netSetupQueueExitParam != 0) {
        reconfigureCode = 49;
    } else if (g_RecoilApp.m_skipIntroFmv != 1) {
        reconfigureCode = 50;
    }

    panel->Destructor();
    FreeZeroPanel(panel);
    RestorePatches(
        patches,
        patchCount
    );
    if (newSessionCode != 0) {
        return newSessionCode;
    }
    if (reconfigureCode != 0) {
        return reconfigureCode;
    }
    return newSessionOk && reconfigureOk ? 0 : 1;
}
