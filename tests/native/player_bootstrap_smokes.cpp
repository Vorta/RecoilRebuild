#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/ai_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

extern "C" unsigned int g_HudUi_InvalidateMask;
extern "C" int g_Player_MissionInitFirstRunFlag;

namespace {
using TestBackendSimpleFn = std::int32_t(__stdcall *)(void *self);
using TestBackendGetStatusFn = std::int32_t(__stdcall *)(void *self, std::int32_t *status);
using TestBackendPlayDirectSoundFn = std::int32_t(__stdcall *)(void *self,
                                                               std::uint32_t reserved1,
                                                               std::uint32_t reserved2,
                                                               std::uint32_t flags);
using TestBackendSetIntFn = std::int32_t(__stdcall *)(void *self, std::int32_t value);

struct TestDirectSoundBufferVTable {
    void *slots00_1c[8];
    void *GetFrequency;
    TestBackendGetStatusFn GetStatus;
    void *slot28;
    void *slot2c;
    TestBackendPlayDirectSoundFn Play;
    TestBackendSetIntFn SetCurrentPosition;
    void *slot38;
    TestBackendSetIntFn SetVolume;
    TestBackendSetIntFn SetPan;
    TestBackendSetIntFn SetFrequency;
    TestBackendSimpleFn Stop;
};

struct TestDirectSoundBuffer {
    TestDirectSoundBufferVTable *vtable;
};

int g_PlayerBootstrapTestPlayCount;
int g_PlayerBootstrapTestStopCount;
int g_PlayerTopMsgPanelAtexitCalls;
void (*g_PlayerTopMsgPanelAtexitCallback)(void);
HudUiStringMenu *g_PlayerBootstrapDebugAuxMenu;
char g_PlayerBootstrapDebugAuxText[23][256];
int g_PlayerBootstrapDebugAuxVisible[23];
int g_PlayerBootstrapDebugAuxSetTextCount[23];
int g_PlayerBootstrapDebugAuxSetVisibleCount[23];

struct PlayerBootstrapCodePatch {
    unsigned char *address;
    unsigned char original[5];
};

void ClearPlayerNodeFlagRestoreGlobalsAtExit() {
    g_PlayerNodeFlagRestoreEntriesBegin = 0;
    g_PlayerNodeFlagRestoreEntriesEnd = 0;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = 0;
}

std::int32_t __stdcall TestDirectSoundGetStatus(void *, std::int32_t *status) {
    *status = 0;
    return 0;
}

std::int32_t __stdcall TestDirectSoundSetInt(void *, std::int32_t) {
    return 0;
}

std::int32_t __stdcall TestDirectSoundPlay(void *, std::uint32_t, std::uint32_t,
                                           std::uint32_t) {
    ++g_PlayerBootstrapTestPlayCount;
    return 0;
}

std::int32_t __stdcall TestDirectSoundStop(void *) {
    ++g_PlayerBootstrapTestStopCount;
    return 0;
}

bool PatchPlayerBootstrapFunctionJump(
    void *target,
    void *replacement,
    PlayerBootstrapCodePatch &patch
) {
    if (target == 0) {
        patch.address = 0;
        return false;
    }

    patch.address = (unsigned char *)target;
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0
    ) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const LONG relativeOffset =
        (LONG)((unsigned char *)replacement - (patch.address + sizeof(patch.original)));
    std::memcpy(patch.address + 1, &relativeOffset, sizeof(relativeOffset));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestorePlayerBootstrapFunctionPatch(
    PlayerBootstrapCodePatch &patch
) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0
    ) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = 0;
}

int FakePlayerTopMsgPanelAtexit(void (*callback)(void)) {
    ++g_PlayerTopMsgPanelAtexitCalls;
    g_PlayerTopMsgPanelAtexitCallback = callback;
    return 0;
}

void ResetPlayerTopMsgPanelAtexitCapture() {
    g_PlayerTopMsgPanelAtexitCalls = 0;
    g_PlayerTopMsgPanelAtexitCallback = 0;
}

bool PlayerTopMsgPanelConstructed(
    const HudUiPanel &panel
) {
    const HudUiElement *const element = (const HudUiElement *)(&panel);
    HudUiPanel probe;
    probe.ConstructorDefault(
        0,
        0,
        0
    );
    const void *const panelTable = *(const void *const *)(&panel);
    const void *const expectedPanelTable = *(const void *const *)(&probe);

    return panelTable == expectedPanelTable &&
           panel.textBuffer[0] == '\0' &&
           element->x == 0 &&
           element->y == 0 &&
           panel.textPick == 0 &&
           panel.textColor0 == 0x00ffffff &&
           panel.textColor1 == 0x00ffffff &&
           panel.hFont != 0 &&
           panel.cachedText[0] == '\0' &&
           panel.textWidthPx == 0 &&
           panel.textHeightPx == 0 &&
           panel.shadowEnabled == 0 &&
           panel.textDirty == 1 &&
           panel.wordWrapEnabled == 0 &&
           panel.wrapRect.left == 0 &&
           panel.wrapRect.top == 0 &&
           panel.wrapRect.right == 0 &&
           panel.wrapRect.bottom == 0;
}

bool PlayerTopMsgPanelDestroyed(
    const HudUiPanel &panel
) {
    HudUiElement probe;
    const void *const panelTable = *(const void *const *)(&panel);
    const void *const expectedCommonTable = *(const void *const *)(&probe);
    return panelTable == expectedCommonTable &&
           panel.textPick == 0;
}

void SavePlayerTopMsgPanel(
    const HudUiPanel &panel,
    unsigned char *saved
) {
    std::memcpy(saved, &panel, sizeof(panel));
}

void RestorePlayerTopMsgPanel(
    HudUiPanel &panel,
    const unsigned char *saved
) {
    std::memcpy(&panel, saved, sizeof(panel));
}

void ClearPlayerTopMsgPanel(
    HudUiPanel &panel
) {
    std::memset(&panel, 0, sizeof(panel));
}

bool FloatNear(
    float actual,
    float expected
) {
    return std::fabs(actual - expected) < 0.0001f;
}

bool Vec3Equals(
    const zVec3 &value,
    const zVec3 &expected
) {
    return FloatNear(value.x, expected.x) &&
           FloatNear(value.y, expected.y) &&
           FloatNear(value.z, expected.z);
}

void MakeAinetReaderStringNode(
    zReader::Node &node,
    const char *value
) {
    node.type = zReader::ZRDR_NODE_STRING;
    node.value.str = const_cast<char *>(value);
}

void MakeAinetReaderFloatNode(
    zReader::Node &node,
    float value
) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeAinetReaderArrayNode(
    zReader::Node &node,
    zReader::Node *payload,
    int count
) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteAinetZrdU32(
    std::FILE *file,
    unsigned int value
) {
    return std::fwrite(&value, sizeof(value), 1, file) == 1;
}

bool WriteAinetZrdNode(
    std::FILE *file,
    const zReader::Node &node
) {
    if (!WriteAinetZrdU32(file, static_cast<unsigned int>(node.type))) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteAinetZrdU32(file, node.value.u32);
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length =
            static_cast<unsigned int>(std::strlen(node.value.str));
        return WriteAinetZrdU32(file, length) &&
               std::fwrite(node.value.str, 1, length, file) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteAinetZrdU32(file, static_cast<unsigned int>(count))) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteAinetZrdNode(file, node.value.nodes[index])) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

struct AinetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountAinetZrdArchive(
    const char *path,
    const AinetZrdArchiveEntry *entries,
    int entryCount,
    zIndexArchive &archive,
    zZarFileRecord *records,
    zArchiveListNode &archiveNode,
    zArchiveList &archiveList
) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 || !WriteAinetZrdNode(file, *entries[index].root)) {
            ok = false;
            break;
        }
        const long endOffset = std::ftell(file);
        if (endOffset < offset) {
            ok = false;
            break;
        }

        records[index] = {};
        records[index].fileOffset = static_cast<unsigned int>(offset);
        records[index].fileSize = static_cast<unsigned int>(endOffset - offset);
        std::strcpy(records[index].name, entries[index].name);
    }

    if (std::fclose(file) != 0 || !ok) {
        std::remove(path);
        return false;
    }

    archive = {};
    archive.hFile = CreateFileA(
        path,
        GENERIC_READ,
        FILE_SHARE_READ,
        nullptr,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );
    if (archive.hFile == INVALID_HANDLE_VALUE) {
        std::remove(path);
        return false;
    }

    archive.recordCount = static_cast<unsigned int>(entryCount);
    archive.records = records;

    archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;

    archiveList = {};
    archiveList.count = 1;
    archiveList.head = &archiveNode;
    g_zArchive_MountedList = &archiveList;
    return true;
}

float PlayerFastSqrtEstimateForTest(
    float value
) {
    std::int32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    bits = (bits >> 1) + 0x1fc00000;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
}

zZbdManager MakePlayerZbdManager(
    zZbdSectionHandlerNode &sentinel
) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearPlayerRegisteredHandlers(
    zZbdSectionHandlerNode &sentinel
) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}

void SetObjectLocalMatrix(
    zClass_Object3DDataPartial *data,
    const zMat4x3 &matrix
) {
    std::memcpy(data->localMatrix, &matrix, sizeof(matrix));
}

template <typename T>
T &PlayerStateFieldAt(
    zUtil_PlayerStateStorage &playerState,
    std::size_t offset
) {
    return *reinterpret_cast<T *>(playerState.bytes + offset);
}

template <typename T>
T &PlayerBootstrapFieldAt(
    void *base,
    std::size_t offset
) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(base) + offset);
}

template <typename Method>
std::uintptr_t PlayerBootstrapMethodAddress(Method method) {
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

int PlayerBootstrapDebugAuxIndex(void *self) {
    if (g_PlayerBootstrapDebugAuxMenu == nullptr) {
        return -1;
    }

    for (int index = 0; index < 23; ++index) {
        if (&g_PlayerBootstrapDebugAuxMenu->items[index] == self) {
            return index;
        }
    }
    return -1;
}

void PlayerBootstrapDebugAuxSetTextFmt(HudUiPanel *self, const char *format, ...) {
    const int index = PlayerBootstrapDebugAuxIndex(self);
    if (index < 0) {
        return;
    }

    va_list args;
    va_start(args, format);
    std::vsnprintf(
        g_PlayerBootstrapDebugAuxText[index],
        sizeof(g_PlayerBootstrapDebugAuxText[index]),
        format,
        args
    );
    va_end(args);
    ++g_PlayerBootstrapDebugAuxSetTextCount[index];
}

struct PlayerBootstrapDebugAuxPanel {
    void SetVisible(int visible) {
        const int index = PlayerBootstrapDebugAuxIndex(this);
        if (index >= 0) {
            g_PlayerBootstrapDebugAuxVisible[index] = visible;
            ++g_PlayerBootstrapDebugAuxSetVisibleCount[index];
        }
    }
};

void InitDestroyedEffectEntry(
    zEffectAnimEntry *entry,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *runtimeNode,
    const char *name
) {
    std::memset(entry, 0, sizeof(*entry));
    std::strcpy(entry->name, name);
    entry->boundNode = boundNode;
    entry->callbackNode = boundNode;
    entry->runtimeNode = runtimeNode;
    entry->priority = 3;
}

HMODULE LoadPlayerBootstrapMessagesDll() {
    HMODULE messagesDll = LoadLibraryA("support\\messages.dll");
    if (messagesDll == nullptr) {
        messagesDll = LoadLibraryA("..\\..\\..\\..\\support\\messages.dll");
    }
    return messagesDll;
}

FILE *OpenPlayerBootstrapNullFile() {
    FILE *file = nullptr;
    fopen_s(&file, "NUL", "w+b");
    if (file == nullptr) {
        file = std::tmpfile();
    }
    return file;
}

void InitPlayerBootstrapShieldWidget(
    HudUiShieldMessageWidget &shield
) {
    new (&shield.widget) HudUiWidget(0);
    new (&shield.percentTextPanel) HudUiPanelSimple;
    new (&shield.meter) HudUiMeter;
    shield.meter.fillPixelsMax = 20;
    shield.meter.points[1].y = 100.0f;
}

int g_CheckpointNetSendCalls;
DWORD g_CheckpointNetSendFlags;
DWORD g_CheckpointNetSendSize;
unsigned char g_CheckpointNetPacketBytes[0x40];

HRESULT __stdcall CheckpointSendFake(
    zNetwork_DPlay4 *,
    DPID,
    DPID,
    DWORD flags,
    LPVOID packet,
    DWORD packetSizeBytes
) {
    ++g_CheckpointNetSendCalls;
    g_CheckpointNetSendFlags = flags;
    g_CheckpointNetSendSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_CheckpointNetPacketBytes)) {
        std::memcpy(
            g_CheckpointNetPacketBytes,
            packet,
            packetSizeBytes
        );
    }
    return 0;
}

struct CheckpointFakeDirectPlay4 {
    void **vtable;
};

void InitCheckpointDirectPlayVtable(
    void **vtable
) {
    std::memset(
        vtable,
        0,
        sizeof(void *) * 52
    );
    vtable[26] = (void *)(&CheckpointSendFake);
}

} // namespace

extern "C" int player_top_msg_panel1_constructor_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel1)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);

    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel1);
    Player_TopMsgPanel1::Constructor();

    const bool ok = PlayerTopMsgPanelConstructed(g_Player_TopMsgPanel1);

    Player_TopMsgPanel1::Destructor();
    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
    return ok ? 0 : 1;
}

extern "C" int player_top_msg_panel1_destructor_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel1)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);

    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel1);
    Player_TopMsgPanel1::Constructor();
    Player_TopMsgPanel1::Destructor();

    const bool ok = PlayerTopMsgPanelDestroyed(g_Player_TopMsgPanel1);

    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
    return ok ? 0 : 1;
}

extern "C" int player_register_top_msg_panel1_on_exit_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel1)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);

    PlayerBootstrapCodePatch patch = {};
    ResetPlayerTopMsgPanelAtexitCapture();
    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel1);
    unsigned char preparedPanel[sizeof(g_Player_TopMsgPanel1)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel1, preparedPanel);

    if (!PatchPlayerBootstrapFunctionJump(
        (void *)(&atexit),
        (void *)(&FakePlayerTopMsgPanelAtexit),
        patch
    )) {
        RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
        return 1;
    }

    Player::RegisterTopMsgPanel1OnExit();
    RestorePlayerBootstrapFunctionPatch(patch);

    const bool ok =
        g_PlayerTopMsgPanelAtexitCalls == 1 &&
        g_PlayerTopMsgPanelAtexitCallback == &Player_TopMsgPanel1::Destructor &&
        std::memcmp(&g_Player_TopMsgPanel1, preparedPanel, sizeof(preparedPanel)) == 0;

    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
    return ok ? 0 : 2;
}

extern "C" int player_init_and_register_top_msg_panel1_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel1)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);

    PlayerBootstrapCodePatch patch = {};
    ResetPlayerTopMsgPanelAtexitCapture();
    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel1);

    if (!PatchPlayerBootstrapFunctionJump(
        (void *)(&atexit),
        (void *)(&FakePlayerTopMsgPanelAtexit),
        patch
    )) {
        RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
        return 1;
    }

    Player::InitAndRegisterTopMsgPanel1();
    RestorePlayerBootstrapFunctionPatch(patch);

    const bool ok =
        PlayerTopMsgPanelConstructed(g_Player_TopMsgPanel1) &&
        g_PlayerTopMsgPanelAtexitCalls == 1 &&
        g_PlayerTopMsgPanelAtexitCallback == &Player_TopMsgPanel1::Destructor;

    Player_TopMsgPanel1::Destructor();
    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel1, oldPanel);
    return ok ? 0 : 2;
}

extern "C" int player_top_msg_panel2_constructor_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel2)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);

    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel2);
    Player_TopMsgPanel2::Constructor();

    const bool ok = PlayerTopMsgPanelConstructed(g_Player_TopMsgPanel2);

    Player_TopMsgPanel2::Destructor();
    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
    return ok ? 0 : 1;
}

extern "C" int player_top_msg_panel2_destructor_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel2)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);

    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel2);
    Player_TopMsgPanel2::Constructor();
    Player_TopMsgPanel2::Destructor();

    const bool ok = PlayerTopMsgPanelDestroyed(g_Player_TopMsgPanel2);

    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
    return ok ? 0 : 1;
}

extern "C" int player_register_top_msg_panel2_cleanup_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel2)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);

    PlayerBootstrapCodePatch patch = {};
    ResetPlayerTopMsgPanelAtexitCapture();
    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel2);
    unsigned char preparedPanel[sizeof(g_Player_TopMsgPanel2)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel2, preparedPanel);

    if (!PatchPlayerBootstrapFunctionJump(
        (void *)(&atexit),
        (void *)(&FakePlayerTopMsgPanelAtexit),
        patch
    )) {
        RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
        return 1;
    }

    Player::RegisterTopMsgPanel2Cleanup();
    RestorePlayerBootstrapFunctionPatch(patch);

    const bool ok =
        g_PlayerTopMsgPanelAtexitCalls == 1 &&
        g_PlayerTopMsgPanelAtexitCallback == &Player_TopMsgPanel2::Destructor &&
        std::memcmp(&g_Player_TopMsgPanel2, preparedPanel, sizeof(preparedPanel)) == 0;

    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
    return ok ? 0 : 2;
}

extern "C" int player_init_and_register_top_msg_panel2_safe_smoke(void) {
    unsigned char oldPanel[sizeof(g_Player_TopMsgPanel2)];
    SavePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);

    PlayerBootstrapCodePatch patch = {};
    ResetPlayerTopMsgPanelAtexitCapture();
    ClearPlayerTopMsgPanel(g_Player_TopMsgPanel2);

    if (!PatchPlayerBootstrapFunctionJump(
        (void *)(&atexit),
        (void *)(&FakePlayerTopMsgPanelAtexit),
        patch
    )) {
        RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
        return 1;
    }

    Player::InitAndRegisterTopMsgPanel2();
    RestorePlayerBootstrapFunctionPatch(patch);

    const bool ok =
        PlayerTopMsgPanelConstructed(g_Player_TopMsgPanel2) &&
        g_PlayerTopMsgPanelAtexitCalls == 1 &&
        g_PlayerTopMsgPanelAtexitCallback == &Player_TopMsgPanel2::Destructor;

    Player_TopMsgPanel2::Destructor();
    RestorePlayerTopMsgPanel(g_Player_TopMsgPanel2, oldPanel);
    return ok ? 0 : 2;
}

extern "C" int player_init_master_common_data_list_smoke(void) {
    PlayerMasterCommonData *const oldHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldTail = g_PlayerMasterCommonDataTail;
    const int oldAux = g_PlayerMasterCommonDataListAux;
    const int oldCount = g_PlayerMasterCommonDataCount;

    PlayerMasterCommonData head = {};
    PlayerMasterCommonData tail = {};
    g_PlayerMasterCommonDataListAux = 1;
    g_PlayerMasterCommonDataHead = &head;
    g_PlayerMasterCommonDataTail = &tail;
    g_PlayerMasterCommonDataCount = 2;

    Player::InitMasterCommonDataList();

    const bool ok = g_PlayerMasterCommonDataListAux == 0 &&
                    g_PlayerMasterCommonDataHead == 0 &&
                    g_PlayerMasterCommonDataTail == 0 &&
                    g_PlayerMasterCommonDataCount == 0;

    g_PlayerMasterCommonDataHead = oldHead;
    g_PlayerMasterCommonDataTail = oldTail;
    g_PlayerMasterCommonDataListAux = oldAux;
    g_PlayerMasterCommonDataCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int player_init_master_modal_data_list_smoke(void) {
    PlayerMasterModalData *const oldHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldTail = g_PlayerMasterModalDataTail;
    const int oldAux = g_PlayerMasterModalDataListAux;
    const int oldCount = g_PlayerMasterModalDataCount;

    PlayerMasterModalData head = {};
    PlayerMasterModalData tail = {};
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataHead = &head;
    g_PlayerMasterModalDataTail = &tail;
    g_PlayerMasterModalDataCount = 2;

    Player::InitMasterModalDataList();

    const bool ok = g_PlayerMasterModalDataListAux == 0 &&
                    g_PlayerMasterModalDataHead == 0 &&
                    g_PlayerMasterModalDataTail == 0 &&
                    g_PlayerMasterModalDataCount == 0;

    g_PlayerMasterModalDataHead = oldHead;
    g_PlayerMasterModalDataTail = oldTail;
    g_PlayerMasterModalDataListAux = oldAux;
    g_PlayerMasterModalDataCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int player_init_projectile_camera_fx_pass3_ui_singleton_smoke(void) {
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;

    g_Player_State7FxPass3Ui = {};
    g_Player_State7FxPass3Ui.clipRectOrNull =
        (HudUiRect *)(&g_Player_State7FxPass3Ui);

    Player::InitProjectileCameraFxPass3UiSingleton();

    const bool ok =
        g_Player_State7FxPass3Ui.clipRectOrNull == 0 &&
        g_Player_State7FxPass3Ui.x == 0 &&
        g_Player_State7FxPass3Ui.y == 0;

    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    return ok ? 0 : 1;
}

extern "C" int player_reset_projectile_camera_fx_pass3_ui_singleton_smoke(void) {
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;

    Player::InitProjectileCameraFxPass3UiSingleton();
    void *const derivedTable = *((void **)(&g_Player_State7FxPass3Ui));
    Player::ResetProjectileCameraFxPass3UiSingleton();

    HudUiElement baseProbe;
    const bool ok = derivedTable != *((void **)(&baseProbe)) &&
                    *((void **)(&g_Player_State7FxPass3Ui)) == *((void **)(&baseProbe));

    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    return ok ? 0 : 1;
}

extern "C" int player_register_projectile_camera_fx_pass3_ui_cleanup_smoke(void) {
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;

    Player::InitProjectileCameraFxPass3UiSingleton();
    void *const oldTable = *((void **)(&g_Player_State7FxPass3Ui));
    Player::RegisterProjectileCameraFxPass3UiCleanup();

    const bool ok = *((void **)(&g_Player_State7FxPass3Ui)) == oldTable;

    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    return ok ? 0 : 1;
}

extern "C" int player_init_and_register_projectile_camera_fx_pass3_ui_singleton_smoke(void) {
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;

    g_Player_State7FxPass3Ui = {};
    g_Player_State7FxPass3Ui.clipRectOrNull =
        (HudUiRect *)(&g_Player_State7FxPass3Ui);

    Player::InitAndRegisterProjectileCameraFxPass3UiSingleton();

    const bool ok =
        g_Player_State7FxPass3Ui.clipRectOrNull == 0 &&
        g_Player_State7FxPass3Ui.x == 0 &&
        g_Player_State7FxPass3Ui.y == 0;

    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    return ok ? 0 : 1;
}

extern "C" int player_destroyed_state_reset_local_finalize_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 4;
    playerState.cameraState = 3;
    playerState.thirdPersonYawOffset = 8.0f;
    playerState.cameraElevationOffset = 9.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    playerState.statusMeterValue = 10.0f;
    commonData.maxHealth = 100.0f;
    commonData.invMaxHealth = 0.01f;
    InitPlayerBootstrapShieldWidget(shield);

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldPrevCameraState = g_PlayerPrevCameraState;
    const int oldPrevSteeringMode = g_PlayerPrevSteeringMode;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;

    HMODULE messagesDll = LoadPlayerBootstrapMessagesDll();
    if (messagesDll == nullptr) {
        return 6;
    }

    int gameControlOptions = 0;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_PlayerStatusMeterRatio = 0.1f;
    g_PlayerPrevCameraState = 3;
    g_PlayerPrevSteeringMode = 1;
    g_HudUi_InvalidateMask = 0;
    g_zLoc_MessagesDllHandle = messagesDll;

    Player::DestroyedStateResetLocalFinalize();

    const bool lifecycleOk = playerState.lifecycleState == 1;
    const bool steeringOk = (gameControlOptions & 0x02) != 0;
    const bool mouseOk = playerState.thirdPersonYawOffset == 0.0f &&
                         playerState.cameraElevationOffset == 0.0f;
    const bool damageOk = playerState.damageProtectionActive == 0 &&
                          playerState.queuedFixedDamageFlag == 0 &&
                          playerState.damageVisualFlag == 0;
    const bool statusOk =
        playerState.statusMeterValue == 100.0f &&
        g_PlayerStatusMeterRatio == 1.0f &&
        std::strcmp(
            &PlayerBootstrapFieldAt<char>(&shield.percentTextPanel, 0x34),
            "100"
        ) == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_PlayerPrevCameraState = oldPrevCameraState;
    g_PlayerPrevSteeringMode = oldPrevSteeringMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    FreeLibrary(messagesDll);
    if (!lifecycleOk) {
        return 1;
    }
    if (!steeringOk) {
        return 2;
    }
    if (!mouseOk) {
        return 3;
    }
    if (!damageOk) {
        return 4;
    }
    return statusOk ? 0 : 5;
}

extern "C" int player_apply_mission_save_data_smoke(void) {
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;

    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData flyModalData = {};
    PlayerModalState flyModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    HudUiShieldMessageWidget shield = {};
    HudUiCounterTextPanel counter = {};
    zVidImagePartial messageImages[40] = {};

    rootNode.classId = 5;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;

    commonData.maxHealth = 200.0f;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.statusMeterValue = 50.0f;
    playerState.nanitePanelLevel = 2;
    playerState.primaryGunGateUntilTime = 99.0f;
    playerState.damageProtectionActive = 1;
    playerState.timedHitStatus.runtimeFlags = 0xffffu;
    playerState.timedHitStatus.currentLevel = 9.0f;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        bank.selectedSide = 0;
        bank.controllerA.weaponBankIndex = bankIndex;
        bank.controllerA.weaponSideIndex = 0;
        bank.controllerA.flags = 0;
        bank.controllerA.ammoOrCharge = 1.0f;
        bank.controllerB.weaponBankIndex = bankIndex;
        bank.controllerB.weaponSideIndex = 1;
        bank.controllerB.flags = 0;
        bank.controllerB.ammoOrCharge = 2.0f;

        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        std::memset(&message, 0, sizeof(message));
        new (&message) HudUiMessage;
        message.variantImages[0] = &messageImages[bankIndex * 4 + 0];
        message.variantImages[1] = &messageImages[bankIndex * 4 + 1];
        message.variantImages[4] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[0] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[1] = &messageImages[bankIndex * 4 + 3];
        PlayerBootstrapFieldAt<int>(&message.panel, 0x2a4) = bankIndex & 1;
    }

    playerState.activeAltGunController = &playerState.altWeaponBanks[1].controllerA;
    playerState.activePrimaryGunController = &playerState.altWeaponBanks[2].controllerB;

    flyModalData.masterType = 1;
    flyModal.masterModalData = &flyModalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &flyModal;
    saveState.modalStateListHead = &flyModal;
    saveState.modalStateListTail = &flyModal;
    saveState.modalStateCount = 1;

    InitPlayerBootstrapShieldWidget(shield);
    counter.Constructor();
    std::memset(&g_HudUiMgrNanitePanel, 0, sizeof(g_HudUiMgrNanitePanel));
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel;

    PlayerMissionSaveData saveData = {};
    saveData.size = sizeof(saveData);
    saveData.altWeaponBankIndex = 3;
    saveData.altWeaponSideIndex = 1;
    saveData.primaryWeaponBankIndex = 4;
    saveData.primaryWeaponSideIndex = 0;
    saveData.playerStatusMeterRatio = 0.375f;
    saveData.hudCounterValue = 345;
    saveData.amphibUnlocked = 1;
    saveData.hoverUnlocked = 0;
    saveData.subUnlocked = 1;
    saveData.aiMode = 7;
    saveData.nextModeSwitchAllowedTime = 12.25f;
    saveData.motionInput = 2;
    saveData.autoTurnSign = -1;
    saveData.bankInput = 3;
    saveData.playerMasterType = 1;
    saveData.cameraTarget = {11.0f, 12.0f, 13.0f};
    saveData.cameraPosition = {21.0f, 22.0f, 23.0f};
    saveData.timedHitStatus.runtimeFlags = 0;
    saveData.timedHitStatus.savedHitSourceEntryId = 77;
    saveData.timedHitStatus.currentLevel = 0.25f;
    saveData.timedHitStatus.targetLevel = 0.5f;
    saveData.timedHitStatus.lightNode = nullptr;
    saveData.timedHitStatus.nextUpdateTime = 8.0f;
    saveData.timedHitStatus.lightParentNode = nullptr;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerMissionSaveWeaponBank &bank = saveData.weaponBank[bankIndex];
        bank.selectedSide = bankIndex & 1;
        bank.sides[0].enabled = bankIndex % 3 == 0 ? 1 : 0;
        bank.sides[0].ammoOrCharge = static_cast<float>(10 + bankIndex);
        bank.sides[1].enabled = bankIndex % 2 == 0 ? 1 : 0;
        bank.sides[1].ammoOrCharge = static_cast<float>(20 + bankIndex);
    }

    g_LocalPlayerSaveState = &saveState;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_MainCamera = &cameraNode;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiMgrObjectiveCounterTextPanel = &counter;
    g_HudUiMgrActiveWeaponMessageIndex = 0;
    g_HudUiMgrActiveWeaponSideIndex = 0;
    g_HudUi_InvalidateMask = 0x80;
    g_PlayerStatusMeterRatio = 0.0f;
    g_Player_HudCounterValue = 0;
    g_Time_AccumulatedTimeSec = 100.0f;

    Player::ApplyMissionSaveData(&saveData);

    int failure = 0;
    if (playerState.altWeaponBanks[3].selectedSide != 1) {
        failure = 1;
    } else if (playerState.activeAltGunController != &playerState.altWeaponBanks[3].controllerB) {
        failure = 6;
    } else if (playerState.activePrimaryGunController !=
               &playerState.altWeaponBanks[4].controllerA) {
        failure = 7;
    } else if (playerState.cachedAltSelectionCode != 301) {
        failure = 8;
    } else if (playerState.cachedPrimarySelectionCode != 400) {
        failure = 9;
    } else if (
        g_PlayerStatusMeterRatio != 0.375f ||
        g_Player_HudCounterValue != 345 ||
        playerState.amphibUnlocked != 1 ||
        playerState.hoverUnlocked != 0 ||
        playerState.subUnlocked != 1 ||
        playerState.aiMode != 7 ||
        playerState.nextModeSwitchAllowedTime != 12.25f ||
        playerState.motionInput != 2 ||
        playerState.autoTurnSign != -1 ||
        playerState.bankInput != 3 ||
        playerState.primaryGunGateUntilTime != 0.0f ||
        playerState.damageProtectionActive != 0
    ) {
        failure = 2;
    } else if (!Vec3Equals(cameraData.targetOrEuler, saveData.cameraTarget) ||
               !Vec3Equals(cameraData.posOffset, saveData.cameraPosition)) {
        failure = 3;
    } else if (
        playerState.timedHitStatus.runtimeFlags != 0 ||
        playerState.timedHitStatus.currentLevel != 0.25f ||
        playerState.timedHitStatus.targetLevel != 0.5f ||
        playerState.timedHitStatus.nextUpdateTime != 8.0f ||
        playerState.timedHitStatus.lightParentNode != &rootNode
    ) {
        failure = 4;
    } else if (
        std::strcmp(&PlayerBootstrapFieldAt<char>(&counter, 0x34), "345") != 0 ||
        shield.meter.points[0].y != 95.0f ||
        g_HudUiMgrNanitePanel.visibleCount != 2
    ) {
        failure = 5;
    }

    for (int bankIndex = 0; failure == 0 && bankIndex < 10; ++bankIndex) {
        const PlayerMissionSaveWeaponBank &savedBank = saveData.weaponBank[bankIndex];
        const PlayerAltWeaponBank &bank = playerState.altWeaponBanks[bankIndex];
        if (
            bank.selectedSide != savedBank.selectedSide ||
            ((bank.controllerA.flags >> 2) & 1) != (savedBank.sides[0].enabled & 1) ||
            ((bank.controllerB.flags >> 2) & 1) != (savedBank.sides[1].enabled & 1) ||
            bank.controllerA.ammoOrCharge != savedBank.sides[0].ammoOrCharge ||
            bank.controllerB.ammoOrCharge != savedBank.sides[1].ammoOrCharge
        ) {
            failure = 20 + bankIndex;
        }
    }

    if (counter.hFont != nullptr) {
        DeleteObject(counter.hFont);
        counter.hFont = nullptr;
    }
    if (shield.percentTextPanel.hFont != nullptr) {
        DeleteObject(shield.percentTextPanel.hFont);
        shield.percentTextPanel.hFont = nullptr;
    }
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_MainCamera = oldMainCamera;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }

    return failure;
}

extern "C" int player_add_scaled_hud_counter_smoke(void) {
    const int oldCounterValue = g_Player_HudCounterValue;
    const int oldPrimaryGunDispatchCount = g_HudSensorTracker.primaryGunDispatchCount;
    const int oldDamageFeedbackHitCount = g_OptCatalog_DamageFeedbackHitCount;

    g_Player_HudCounterValue = 10;
    g_HudSensorTracker.primaryGunDispatchCount = 0;
    g_OptCatalog_DamageFeedbackHitCount = 7;

    Player::AddScaledHudCounterValue(1.25f);
    int failure = 0;
    if (g_Player_HudCounterValue != 1260) {
        failure = 1;
    }

    if (failure == 0) {
        g_HudSensorTracker.primaryGunDispatchCount = 4;
        g_OptCatalog_DamageFeedbackHitCount = 3;
        Player::AddScaledHudCounterValue(2.0f);
        if (g_Player_HudCounterValue != 2760) {
            failure = 2;
        }
    }

    if (failure == 0) {
        g_HudSensorTracker.primaryGunDispatchCount = 4;
        g_OptCatalog_DamageFeedbackHitCount = 1;
        Player::AddScaledHudCounterValue(-1.5f);
        if (g_Player_HudCounterValue != 2385) {
            failure = 3;
        }
    }

    g_Player_HudCounterValue = oldCounterValue;
    g_HudSensorTracker.primaryGunDispatchCount = oldPrimaryGunDispatchCount;
    g_OptCatalog_DamageFeedbackHitCount = oldDamageFeedbackHitCount;
    return failure;
}

extern "C" int player_update_debug_overlay_hud_smoke(void) {
    HudUiStringMenu *const oldStringMenu = g_HudUiMgrStringMenu;
    HudUiCounterTextPanel *const oldObjectiveCounter = g_HudUiMgrObjectiveCounterTextPanel;
    const int oldReticleMode = g_HudUiMgrReticleMode;
    const int oldHudCounterValue = g_Player_HudCounterValue;
    HudUiMessage oldMessages[10] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
        g_HudUiMgrMessages[index] = {};
        PlayerBootstrapFieldAt<int>(&g_HudUiMgrMessages[index].panel, 0x2a4) = 0;
    }

    HudUiCounterTextPanel counter = {};
    counter.Constructor();
    g_HudUiMgrObjectiveCounterTextPanel = &counter;

    std::uintptr_t auxTable[32] = {};
    auxTable[24] =
        PlayerBootstrapMethodAddress(&PlayerBootstrapDebugAuxPanel::SetVisible);
    auxTable[29] =
        reinterpret_cast<std::uintptr_t>(&PlayerBootstrapDebugAuxSetTextFmt);

    HudUiStringMenu menu = {};
    for (int index = 0; index < 23; ++index) {
        *reinterpret_cast<std::uintptr_t **>(&menu.items[index]) = auxTable;
    }
    g_HudUiMgrStringMenu = &menu;
    g_PlayerBootstrapDebugAuxMenu = &menu;
    std::memset(g_PlayerBootstrapDebugAuxText, 0, sizeof(g_PlayerBootstrapDebugAuxText));
    std::memset(
        g_PlayerBootstrapDebugAuxVisible,
        0,
        sizeof(g_PlayerBootstrapDebugAuxVisible)
    );
    std::memset(
        g_PlayerBootstrapDebugAuxSetTextCount,
        0,
        sizeof(g_PlayerBootstrapDebugAuxSetTextCount)
    );
    std::memset(
        g_PlayerBootstrapDebugAuxSetVisibleCount,
        0,
        sizeof(g_PlayerBootstrapDebugAuxSetVisibleCount)
    );

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    PlayerGunFireController altController = {};
    OptCatalogEntryDef altEntry = {};
    zClass_NodePartial rootNode = {};
    AINetNode currentPathNode = {};

    std::strcpy(rootNode.name, "DebugRoot");
    currentPathNode.nodeIndex = 42;
    modalData.masterType = 3;
    modalState.masterModalData = &modalData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.rootNode = &rootNode;
    playerState.activeAltGunController = &altController;
    playerState.activePrimaryGunController = nullptr;
    playerState.lifecycleState = 1;
    playerState.slipSfxActive = 1;
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.worldPos = {12.75f, -4.25f, 100.5f};
    playerState.restartYawRad = 1.0f;
    playerState.aimTargetDistanceApprox = 20.0f;
    altEntry.range = 25.0f;
    altController.optCatalogEntry = &altEntry;
    altController.weaponBankIndex = 3;
    altController.weaponSideIndex = 1;
    altController.ammoOrCharge = 7.5f;
    g_Player_HudCounterValue = 77;
    g_HudUiMgrReticleMode = 0;

    Player::UpdateDebugOverlayHud(&saveState, 123, 456);

    int failure = 0;
    if (g_HudUiMgrReticleMode != 1) {
        failure = 1;
    } else if (std::strcmp(&PlayerBootstrapFieldAt<char>(&counter, 0x34), "77") != 0) {
        failure = 2;
    } else if (
        g_PlayerBootstrapDebugAuxSetTextCount[1] != 1 ||
        g_PlayerBootstrapDebugAuxSetVisibleCount[1] != 1 ||
        g_PlayerBootstrapDebugAuxVisible[1] != 1 ||
        std::strcmp(
            g_PlayerBootstrapDebugAuxText[1],
            "DebugRoot using TRACK dynamics - S"
        ) != 0
    ) {
        failure = 3;
    } else if (
        g_PlayerBootstrapDebugAuxSetTextCount[2] != 1 ||
        g_PlayerBootstrapDebugAuxSetVisibleCount[2] != 1 ||
        g_PlayerBootstrapDebugAuxVisible[2] != 1 ||
        std::strcmp(g_PlayerBootstrapDebugAuxText[2], "POS 12 -4 100 YAW 57") != 0
    ) {
        failure = 4;
    }

    if (reinterpret_cast<HudUiPanel *>(&counter)->hFont != nullptr) {
        DeleteObject(reinterpret_cast<HudUiPanel *>(&counter)->hFont);
        reinterpret_cast<HudUiPanel *>(&counter)->hFont = nullptr;
    }
    g_HudUiMgrStringMenu = oldStringMenu;
    g_HudUiMgrObjectiveCounterTextPanel = oldObjectiveCounter;
    g_HudUiMgrReticleMode = oldReticleMode;
    g_Player_HudCounterValue = oldHudCounterValue;
    g_PlayerBootstrapDebugAuxMenu = nullptr;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }

    return failure;
}

extern "C" int player_destroyed_state_reset_finalize_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    alignas(GameNetPlayerRow) unsigned char rowStorage[sizeof(GameNetPlayerRow)] = {};
    GameNetPlayerRow *const row = reinterpret_cast<GameNetPlayerRow *>(rowStorage);
    HudUiShieldMessageWidget shield = {};
    HudUiTextStack4 topStack = {};

    saveState.playerState = &playerState;
    saveState.netPlayerRow = row;
    row->saveState = reinterpret_cast<GameNetPlayerSaveState *>(&saveState);
    playerState.masterCommonData = &commonData;
    playerState.lifecycleState = 4;
    playerState.cameraState = 3;
    playerState.cameraTransitionTimer = 123;
    playerState.statusMeterValue = 10.0f;
    commonData.maxHealth = 120.0f;
    commonData.invMaxHealth = 1.0f / 120.0f;
    row->playerColorIndex = 1;
    InitPlayerBootstrapShieldWidget(shield);

    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const int oldRowCount = g_GameNetPlayerRowCount;
    HMODULE const oldMessagesDll = g_zLoc_MessagesDllHandle;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    const int oldPrevCameraState = g_PlayerPrevCameraState;
    const int oldPrevSteeringMode = g_PlayerPrevSteeringMode;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;

    HMODULE messagesDll = LoadPlayerBootstrapMessagesDll();
    if (messagesDll == nullptr) {
        return 6;
    }

    int gameControlOptions = 0;
    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_HudUiMgrShieldMessageWidget = &shield;
    g_HudUiTopMessageStack = &topStack;
    g_GameNetPlayerRowHead = row;
    g_GameNetPlayerRowTail = row;
    g_GameNetPlayerRowCount = 1;
    g_zLoc_MessagesDllHandle = messagesDll;
    g_PlayerStatusMeterRatio = 10.0f / 120.0f;
    g_PlayerPrevCameraState = 3;
    g_PlayerPrevSteeringMode = 1;
    g_HudUi_InvalidateMask = 0;

    Player::DestroyedStateResetFinalizeCallback(&saveState);

    int result = 0;
    if (playerState.lifecycleState != 1) {
        result = 10;
    } else if (playerState.statusMeterValue != 120.0f) {
        result = 11;
    } else if (playerState.cameraTransitionTimer != 0) {
        result = 12;
    } else if ((gameControlOptions & 0x02) == 0) {
        result = 13;
    } else if (g_PlayerStatusMeterRatio != 1.0f) {
        result = 14;
    } else if (std::strcmp(
                   &PlayerBootstrapFieldAt<char>(&shield.percentTextPanel, 0x34),
                   "100"
               ) != 0) {
        result = 15;
    }

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_zLoc_MessagesDllHandle = oldMessagesDll;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_PlayerPrevCameraState = oldPrevCameraState;
    g_PlayerPrevSteeringMode = oldPrevSteeringMode;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    FreeLibrary(messagesDll);
    return result;
}

extern "C" int player_clear_respawn_transition_flag_callback_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.cameraTransitionTimer = 17;

    Player::ClearRespawnTransitionFlagCallback(&saveState);
    return playerState.cameraTransitionTimer == 0 ? 0 : 1;
}

extern "C" int player_destroyed_state_reset_callback_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldLocalPlayerSaveState = g_LocalPlayerSaveState;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    HudLayoutBase *const oldCurrentLayout = g_HudUiMgrCurrentLayout;
    HudUiShieldMessageWidget *const oldShieldWidget = g_HudUiMgrShieldMessageWidget;
    const std::uint32_t oldInvalidateMask = g_HudUi_InvalidateMask;
    const int oldActiveWeaponMessageIndex = g_HudUiMgrActiveWeaponMessageIndex;
    const int oldActiveWeaponSideIndex = g_HudUiMgrActiveWeaponSideIndex;
    const HudUiNanitePanel oldNanitePanel = g_HudUiMgrNanitePanel;
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;
    HudUiMessage oldMessages[10] = {};
    HudUiCounter oldModeCounters[4] = {};
    for (int index = 0; index < 10; ++index) {
        oldMessages[index] = g_HudUiMgrMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        oldModeCounters[index] = g_HudUiMgrModeCounters[index];
    }

    zClass_Object3D_ModelRefLerpQueue::Reset();

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker.missionId = 8;
    g_HudSensorTracker.raceCheckpointMode = 1;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterCommonData localCommonData = {};
    PlayerMasterModalData sourceData = {};
    PlayerMasterModalData trackData = {};
    PlayerModalState sourceModal = {};
    PlayerModalState trackModal = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial modeVariantNode = {};
    zClass_NodePartial runtimeNode = {};
    zClass_Object3DDataPartial rootData = {};
    zEffectAnimEntry destroyedRespawn = {};
    HudUiShieldMessageWidget shield = {};
    zVidImagePartial messageImages[40] = {};
    zVidImagePartial counterImages[6] = {};
    zVidTexturePackEntry builtinTexturePack = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &sourceModal;
    saveState.modalStateListHead = &sourceModal;
    saveState.modalStateListTail = &trackModal;
    saveState.modalStateCount = 2;
    localSaveState.playerState = &localPlayerState;
    sourceModal.masterModalData = &sourceData;
    sourceModal.next = &trackModal;
    trackModal.masterModalData = &trackData;
    sourceData.masterType = 1;
    trackData.masterType = 3;
    playerState.masterCommonData = &commonData;
    localPlayerState.masterCommonData = &localCommonData;
    commonData.maxHealth = 120.0f;
    commonData.invMaxHealth = 1.0f / 120.0f;
    localCommonData.maxHealth = 120.0f;
    localCommonData.invMaxHealth = 1.0f / 120.0f;

    rootNode.classId = 5;
    rootNode.classData = &rootData;
    std::strcpy(rootNode.name, "destroyed_root");
    modeVariantNode.classId = 2;
    playerState.rootNode = &rootNode;
    playerState.modeVariantNode = &modeVariantNode;
    InitDestroyedEffectEntry(&destroyedRespawn, &rootNode, &runtimeNode, "destroyed_respawn");
    destroyedRespawn.activationState = 2;
    playerState.destroyedRespawnFxEntry = &destroyedRespawn;
    playerState.statusMeterValue = 5.0f;
    playerState.damageProtectionActive = 1;
    playerState.queuedFixedDamageFlag = 1;
    playerState.damageVisualFlag = 1;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.aiMode = 9;
    playerState.nextModeSwitchAllowedTime = 7.0f;
    playerState.motionInput = 2;
    playerState.autoTurnSign = -1;
    playerState.thirdPersonYawOffset = 4.0f;
    playerState.cameraElevationOffset = 5.0f;
    playerState.localVel = {1.0f, 2.0f, 3.0f};
    playerState.projectileSpawnVel = {4.0f, 5.0f, 6.0f};
    playerState.yawRotatedLocalVel = {7.0f, 8.0f, 9.0f};
    playerState.angVelPitch = 1.5f;
    playerState.angVelYaw = 2.5f;
    playerState.angVelRoll = 3.5f;
    playerState.steeringInput = 0.5f;
    playerState.throttleInput = 0.6f;
    playerState.subVerticalInput = 0.7f;

    InitPlayerBootstrapShieldWidget(shield);
    g_HudUiMgrShieldMessageWidget = &shield;

    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        HudUiMessage &message = g_HudUiMgrMessages[bankIndex];
        std::memset(&message, 0, sizeof(message));
        new (&message) HudUiMessage;
        message.variantImages[0] = &messageImages[bankIndex * 4 + 0];
        message.variantImages[1] = &messageImages[bankIndex * 4 + 1];
        message.variantImages[4] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[0] = &messageImages[bankIndex * 4 + 2];
        message.sideImageSwaps[1] = &messageImages[bankIndex * 4 + 3];
    }

    std::memset(&g_HudUiMgrNanitePanel, 0, sizeof(g_HudUiMgrNanitePanel));
    new (&g_HudUiMgrNanitePanel) HudUiNanitePanel;

    for (int index = 1; index < 4; ++index) {
        std::memset(&g_HudUiMgrModeCounters[index], 0, sizeof(g_HudUiMgrModeCounters[index]));
        new (&g_HudUiMgrModeCounters[index]) HudUiCounter;
        PlayerBootstrapFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xbc) =
            &counterImages[(index - 1) * 2];
        PlayerBootstrapFieldAt<zVidImagePartial *>(&g_HudUiMgrModeCounters[index], 0xc0) =
            &counterImages[(index - 1) * 2 + 1];
    }

    g_GameStateOrMapTable =
        reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSaveState);
    g_LocalPlayerSaveState = &localSaveState;
    g_GameNetStatus_AllowMaps = 1;
    g_GameNetSpawnPointHead = nullptr;
    g_GameNetSpawnPointTail = nullptr;
    g_GameNetSpawnPointCount = 0;
    g_HudUiMgrCurrentLayout = nullptr;
    g_HudUi_InvalidateMask = 0x80;
    std::strcpy(builtinTexturePack.filePath, "test.zbd");
    builtinTexturePack.fileHandle = OpenPlayerBootstrapNullFile();
    g_zVid_BuiltinTexturePacks = &builtinTexturePack;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zImage_TexDirEntryCount = 0;

    Player::DestroyedStateResetCallback(nullptr, &saveState, 0);

    zClass_Object3D_ModelRefLerpTask *const fadeTask = g_ModelRefLerpQueueState.head;
    zZbdSectionHandlerNode *const minesNode = sentinel.next;
    const bool effectOk =
        destroyedRespawn.activationState == 0 && (destroyedRespawn.flags & 0x40u) != 0;
    const bool damageOk =
        playerState.damageProtectionActive == 0 &&
        playerState.queuedFixedDamageFlag == 0 &&
        playerState.damageVisualFlag == 0;
    const bool objectOk =
        playerState.statusMeterValue == 120.0f &&
        (rootData.flags & 0x02) != 0 &&
        rootData.alphaScale == 0.0f &&
        g_ModelRefLerpQueueState.count == 1 &&
        fadeTask != nullptr &&
        fadeTask->node == &rootNode &&
        fadeTask->callbackCtx == &saveState &&
        fadeTask->onComplete == (void *)(&Player::DestroyedStateResetFinalizeCallback) &&
        fadeTask->currentModelRef == 0.0f &&
        fadeTask->targetModelRef == 1.0f &&
        fadeTask->modelRefDeltaPerSec == 1.0f;
    const bool motionOk =
        playerState.aiMode == 0 &&
        playerState.nextModeSwitchAllowedTime == 0.0f &&
        playerState.motionInput == 0 &&
        playerState.autoTurnSign == 0 &&
        playerState.thirdPersonYawOffset == 0.0f &&
        playerState.cameraElevationOffset == 0.0f &&
        Vec3Equals(playerState.localVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.projectileSpawnVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.yawRotatedLocalVel, {0.0f, 0.0f, 0.0f}) &&
        playerState.angVelPitch == 0.0f &&
        playerState.angVelYaw == 0.0f &&
        playerState.angVelRoll == 0.0f;
    const bool transitionOk =
        saveState.primaryModalState == &trackModal &&
        playerState.currentMasterType == 1 &&
        (modeVariantNode.flags & 0x04) != 0;
    const bool weaponOk =
        playerState.activeAltGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.activePrimaryGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.cachedAltSelectionCode == 100 &&
        playerState.cachedPrimarySelectionCode == 100 &&
        playerState.timedHitStatus.runtimeFlags == 0;
    const bool hudOk =
        std::strcmp(&PlayerBootstrapFieldAt<char>(&shield.percentTextPanel, 0x34), "100") == 0;
    const bool zbdOk =
        minesNode != &sentinel &&
        minesNode->sectionHandler.sectionName != nullptr &&
        std::strcmp(minesNode->sectionHandler.sectionName, "Mines") == 0;

    int result = 0;
    if (destroyedRespawn.activationState != 1) {
        result = 20 + destroyedRespawn.activationState;
    } else if ((destroyedRespawn.flags & 0x40u) == 0) {
        result = 29;
    } else if (!damageOk) {
        result = 2;
    } else if (!objectOk) {
        result = 3;
    } else if (!motionOk) {
        result = 4;
    } else if (!transitionOk) {
        result = 5;
    } else if (!weaponOk) {
        result = 6;
    } else if (!hudOk) {
        result = 7;
    } else if (!zbdOk) {
        result = 8;
    }

    zClass_Object3D_ModelRefLerpQueue::Reset();
    ClearPlayerRegisteredHandlers(sentinel);
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zUtil_ZbdManager = oldZbdManager;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_LocalPlayerSaveState = oldLocalPlayerSaveState;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_HudUiMgrCurrentLayout = oldCurrentLayout;
    g_HudUiMgrShieldMessageWidget = oldShieldWidget;
    g_HudUi_InvalidateMask = oldInvalidateMask;
    g_HudUiMgrActiveWeaponMessageIndex = oldActiveWeaponMessageIndex;
    g_HudUiMgrActiveWeaponSideIndex = oldActiveWeaponSideIndex;
    g_HudUiMgrNanitePanel = oldNanitePanel;
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    for (int index = 0; index < 10; ++index) {
        g_HudUiMgrMessages[index] = oldMessages[index];
    }
    for (int index = 0; index < 4; ++index) {
        g_HudUiMgrModeCounters[index] = oldModeCounters[index];
    }
    if (builtinTexturePack.fileHandle != nullptr) {
        std::fclose(builtinTexturePack.fileHandle);
    }
    return result;
}

extern "C" int player_destroyed_state_respawn_callback_smoke(void) {
    zVidTexturePackEntry *const oldBuiltinTexturePacks = g_zVid_BuiltinTexturePacks;
    const int oldBuiltinTexturePackCount = g_zVid_BuiltinTexturePackCount;
    const int oldTexDirEntryCount = g_zImage_TexDirEntryCount;

    zClass_Object3D_ModelRefLerpQueue::Reset();

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial healthyNode = {};
    zClass_NodePartial *rootChildren[] = {&healthyNode};
    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial healthyData = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    std::strcpy(rootNode.name, "root");
    std::strcpy(healthyNode.name, "healthy");
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.listCountB = 1;
    rootNode.listB = rootChildren;
    healthyNode.classId = 5;
    healthyNode.classData = &healthyData;
    healthyNode.flags = 0x01;
    healthyData.localMatrix[9] = 4.0f;
    healthyData.localMatrix[10] = 5.0f;
    healthyData.localMatrix[11] = 6.0f;
    healthyData.rotation = {7.0f, 8.0f, 9.0f};
    commonData.maxHealth = 90.0f;
    playerState.statusMeterValue = 12.0f;
    playerState.damageProtectionActive = 3;
    playerState.queuedFixedDamageFlag = 4;
    playerState.damageVisualFlag = 5;
    playerState.cachedAltSelectionCode = 201;
    playerState.cachedPrimarySelectionCode = 100;

    zVidTexturePackEntry builtinTexturePack = {};
    builtinTexturePack.fileHandle = OpenPlayerBootstrapNullFile();
    if (builtinTexturePack.fileHandle == nullptr) {
        g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
        g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
        g_zImage_TexDirEntryCount = oldTexDirEntryCount;
        return 2;
    }
    g_zVid_BuiltinTexturePacks = &builtinTexturePack;
    g_zVid_BuiltinTexturePackCount = 1;
    g_zImage_TexDirEntryCount = 0;

    Player::DestroyedStateRespawnCallback(nullptr, &saveState, 0);

    zClass_Object3D_ModelRefLerpTask *const fadeTask = g_ModelRefLerpQueueState.head;
    const bool objectOk =
        (rootData.flags & 0x02) != 0 &&
        rootData.alphaScale == 0.0f &&
        g_ModelRefLerpQueueState.count == 1 &&
        fadeTask != nullptr &&
        fadeTask->node == &rootNode &&
        fadeTask->callbackCtx == &saveState &&
        fadeTask->onComplete == (void *)(&Player::ClearRespawnTransitionFlagCallback) &&
        fadeTask->currentModelRef == 0.0f &&
        fadeTask->targetModelRef == 1.0f &&
        FloatNear(fadeTask->modelRefDeltaPerSec, 0.2f) &&
        healthyData.localMatrix[9] == 0.0f &&
        healthyData.localMatrix[10] == 0.0f &&
        healthyData.localMatrix[11] == 0.0f &&
        healthyData.rotation.x == 0.0f &&
        healthyData.rotation.y == 0.0f &&
        healthyData.rotation.z == 0.0f;
    const bool stateOk =
        playerState.damageProtectionActive == 0 &&
        playerState.queuedFixedDamageFlag == 0 &&
        playerState.damageVisualFlag == 0 &&
        playerState.statusMeterValue == 90.0f &&
        playerState.cachedAltSelectionCode == 0 &&
        playerState.cachedPrimarySelectionCode == 0;

    const bool ok = objectOk && stateOk;
    zClass_Object3D_ModelRefLerpQueue::Reset();
    g_zVid_BuiltinTexturePacks = oldBuiltinTexturePacks;
    g_zVid_BuiltinTexturePackCount = oldBuiltinTexturePackCount;
    g_zImage_TexDirEntryCount = oldTexDirEntryCount;
    return ok ? 0 : 1;
}

extern "C" int ainet_find_by_net_id_smoke(void) {
    AINet first = {};
    AINet second = {};
    AINet third = {};
    first.netId = 10;
    second.netId = 20;
    third.netId = 30;
    first.next = &second;
    second.next = &third;

    AINet *const oldHead = g_AINetListHead;
    g_AINetListHead = &first;

    const bool ok = AINet::FindByNetId(20) == &second &&
                    AINet::FindByNetId(99) == 0;
    g_AINetListHead = 0;
    const bool emptyOk = AINet::FindByNetId(10) == 0;

    g_AINetListHead = oldHead;
    return ok && emptyOk ? 0 : 1;
}

extern "C" int ainet_find_nearest_node_smoke(void) {
    AINetNode first = {};
    AINetNode second = {};
    AINetNode third = {};
    first.position.x = 10.0f;
    first.position.y = 0.0f;
    first.position.z = 0.0f;
    second.position.x = 1.0f;
    second.position.y = 2.0f;
    second.position.z = 3.0f;
    third.position.x = -2.0f;
    third.position.y = 0.0f;
    third.position.z = 1.0f;
    first.next = &second;
    second.next = &third;

    const zVec3 query = {0.0f, 0.0f, 0.0f};
    return AINet::FindNearestNode(
        &query,
        &first
    ) == &third &&
                   AINet::FindNearestNode(
                       &query,
                       0
                   ) == 0
               ? 0
               : 1;
}

extern "C" int checkpoint_update_player_lap_progress_and_notify_net_smoke(void) {
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const int oldCheckpointCount = g_HudSensorTracker.checkpointCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    void *vtable[52];
    InitCheckpointDirectPlayVtable(vtable);
    CheckpointFakeDirectPlay4 dplay = {vtable};
    zNetwork_PlayerRecord localPlayer = {};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = (zNetwork_DPlay4 *)(&dplay);
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x1234;
    g_zNetwork_IsHostFlag = 0;
    g_CheckpointNetSendCalls = 0;
    g_CheckpointNetSendFlags = 0;
    g_CheckpointNetSendSize = 0;
    std::memset(
        g_CheckpointNetPacketBytes,
        0,
        sizeof(g_CheckpointNetPacketBytes)
    );

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    g_HudSensorTracker.checkpointCount = 3;

    PlayerStateFieldAt<int>(playerState, 0x1020) = 1;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 2);
    if (PlayerStateFieldAt<int>(playerState, 0x1020) != 1 ||
        g_CheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 1;
    }

    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 1);
    if (PlayerStateFieldAt<int>(playerState, 0x101c) != 1 ||
        g_CheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 2;
    }

    PlayerStateFieldAt<int>(playerState, 0x1020) = 0;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 3);
    if (PlayerStateFieldAt<int>(playerState, 0x101c) != 0 ||
        PlayerStateFieldAt<int>(playerState, 0x1020) != 0 ||
        PlayerStateFieldAt<int>(playerState, 0x1024) != 0 ||
        g_CheckpointNetSendCalls != 0) {
        g_Time_AccumulatedTimeSec = oldAccumulatedTime;
        g_HudSensorTracker.checkpointCount = oldCheckpointCount;
        g_zNetwork_pDirectPlay4 = oldDPlay;
        g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
        g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
        g_zNetwork_IsHostFlag = oldIsHost;
        return 3;
    }

    PlayerStateFieldAt<int>(playerState, 0x101c) = 1;
    PlayerStateFieldAt<int>(playerState, 0x1020) = 1;
    PlayerStateFieldAt<float>(playerState, 0x10a4) = 35.0f;
    PlayerStateFieldAt<float>(playerState, 0x10a8) = 10.0f;
    PlayerStateFieldAt<int>(playerState, 0x10ac) = 7;
    g_Time_AccumulatedTimeSec = 50.0f;
    Checkpoint::UpdatePlayerLapProgressAndNotifyNet(&saveState, 3);

    const NetPkt0E_PlayerLapProgress *const packet =
        reinterpret_cast<const NetPkt0E_PlayerLapProgress *>(g_CheckpointNetPacketBytes);
    const bool ok =
        PlayerStateFieldAt<int>(playerState, 0x101c) == 0 &&
        PlayerStateFieldAt<int>(playerState, 0x1020) == 0 &&
        PlayerStateFieldAt<int>(playerState, 0x1024) == 0 &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x109c), 15.0f) &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x10a0), 40.0f) &&
        FloatNear(PlayerStateFieldAt<float>(playerState, 0x10a4), 50.0f) &&
        PlayerStateFieldAt<int>(playerState, 0x10ac) == 8 &&
        g_CheckpointNetSendCalls == 1 &&
        g_CheckpointNetSendFlags == 1 &&
        g_CheckpointNetSendSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        packet->header.packetType == 0x0e &&
        packet->header.payloadDword0 == 0x1234 &&
        packet->lapCountPacked == 8 &&
        FloatNear(packet->lapTimeSec, 40.0f);

    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_HudSensorTracker.checkpointCount = oldCheckpointCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    return ok ? 0 : 4;
}

extern "C" int checkpoint_instantiate_named_objects_smoke(void) {
    const int oldCheckpointCount = g_HudSensorTracker.checkpointCount;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);

    zClass_NodePartial checkpoint1 = {};
    zClass_NodePartial checkpoint2 = {};
    zClass_NodePartial child = {};
    zClass_NodePartial grandchild = {};
    zClass_NodePartial unrelated = {};
    zClass_NodePartial *checkpoint2Children[] = {&child};
    zClass_NodePartial *childChildren[] = {&grandchild};
    zClass_TypeListLink checkpoint1Link = {&checkpoint1, nullptr, nullptr, 0};
    zClass_TypeListLink checkpoint2Link = {&checkpoint2, &checkpoint1Link, nullptr, 0};
    zClass_TypeListLink unrelatedLink = {&unrelated, &checkpoint2Link, nullptr, 0};

    std::strcpy(checkpoint1.name, "checkpoint1");
    std::strcpy(checkpoint2.name, "checkpoint2");
    std::strcpy(unrelated.name, "checkpoint4");
    checkpoint2.listCountB = 1;
    checkpoint2.listB = checkpoint2Children;
    child.listCountB = 1;
    child.listB = childChildren;

    checkpoint1Link.next = &checkpoint2Link;
    checkpoint2Link.next = &unrelatedLink;
    zClass_TypeList::Head(6) = &checkpoint1Link;
    zClass_TypeList::Tail(6) = &unrelatedLink;
    g_HudSensorTracker.checkpointCount = 3;

    Checkpoint::InstantiateNamedObjects();

    const bool checkpoint1Ok =
        checkpoint1.auxFlags == 2 &&
        (checkpoint1.flags & 0x40000) != 0 &&
        checkpoint1.callbackContext == &checkpoint1 &&
        (checkpoint1.flags & 0x200000) != 0;
    const bool checkpoint2Ok =
        checkpoint2.auxFlags == 2 &&
        child.auxFlags == 2 &&
        grandchild.auxFlags == 2 &&
        (checkpoint2.flags & 0x40000) != 0 &&
        (child.flags & 0x40000) != 0 &&
        (grandchild.flags & 0x40000) != 0 &&
        checkpoint2.callbackContext == &checkpoint2 &&
        child.callbackContext == &checkpoint2 &&
        grandchild.callbackContext == &checkpoint2 &&
        (checkpoint2.flags & 0x200000) != 0 &&
        (child.flags & 0x200000) != 0 &&
        (grandchild.flags & 0x200000) != 0;
    const bool unrelatedOk =
        unrelated.auxFlags == 0 &&
        unrelated.flags == 0 &&
        unrelated.callbackContext == nullptr;

    g_HudSensorTracker.checkpointCount = oldCheckpointCount;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    return checkpoint1Ok && checkpoint2Ok && unrelatedOk ? 0 : 1;
}

extern "C" int player_ai_discard_negative_branch_nodes_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode *const negativeA = static_cast<AINetNode *>(std::calloc(1, sizeof(AINetNode)));
    AINetNode *const negativeB = static_cast<AINetNode *>(std::calloc(1, sizeof(AINetNode)));
    AINetNode positive = {};

    if (negativeA == 0 || negativeB == 0) {
        std::free(negativeA);
        std::free(negativeB);
        return 1;
    }

    negativeA->nodeIndex = -2;
    negativeB->nodeIndex = -1;
    positive.nodeIndex = 3;
    negativeA->neighborNodes[0] = negativeB;
    negativeB->neighborNodes[0] = &positive;
    playerState.aiCurrentPathNode = negativeA;
    saveState.playerState = &playerState;

    AINet::AiDiscardNegativeBranchPathNodes(&saveState);
    return playerState.aiCurrentPathNode == &positive ? 0 : 1;
}

extern "C" int player_get_save_state_list_head_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState saveState = {};

    g_PlayerSaveStateListHead = 0;
    const bool nullOk = Player::GetSaveStateListHead() == 0;

    g_PlayerSaveStateListHead = &saveState;
    const bool valueOk = Player::GetSaveStateListHead() == &saveState;

    g_PlayerSaveStateListHead = oldHead;
    return nullOk && valueOk ? 0 : 1;
}

extern "C" int player_init_save_state_list_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldTail = g_PlayerSaveStateListTail;
    const int oldAux = g_PlayerSaveStateListAux;
    const int oldCount = g_PlayerSaveStateCount;

    zUtil_SaveGameState head = {};
    zUtil_SaveGameState tail = {};
    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateListHead = &head;
    g_PlayerSaveStateListTail = &tail;
    g_PlayerSaveStateCount = 2;

    Player::InitSaveStateList();

    const bool ok = g_PlayerSaveStateListAux == 0 &&
                    g_PlayerSaveStateListHead == 0 &&
                    g_PlayerSaveStateListTail == 0 &&
                    g_PlayerSaveStateCount == 0;

    g_PlayerSaveStateListHead = oldHead;
    g_PlayerSaveStateListTail = oldTail;
    g_PlayerSaveStateListAux = oldAux;
    g_PlayerSaveStateCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int player_clone_type6_node_from_template_and_rename_smoke(void) {
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    int *const oldNetworkEnabledPtr = ZOPT_NETWORK_ENABLED;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zClass_NodePartial world = {};
    zClass_NodePartial templateNode = {};
    zClass_TypeListLink templateLink = {&templateNode, 0, 0, 0};

    std::strcpy(templateNode.name, "template_lod");
    templateNode.classId = 6;
    templateNode.flags = 0x04000080;
    templateNode.gridCol = -1;
    templateNode.gridRow = -1;
    world.classId = 2;

    zClass_TypeList::Head(6) = &templateLink;
    zClass_TypeList::Tail(6) = &templateLink;
    g_Player_RuntimeDiScene = &world;

    zClass_NodePartial *const missing =
        Player::CloneType6NodeFromTemplateAndRename("missing_template", "unused");
    zClass_NodePartial *const cloned =
        Player::CloneType6NodeFromTemplateAndRename("template_lod", "runtime_lod");

    const bool ok =
        missing == 0 && cloned == &templateNode &&
        std::strcmp(templateNode.name, "runtime_lod") == 0 &&
        (templateNode.flags & 0x04) != 0 &&
        world.listCountB == 1 &&
        world.listB != 0 &&
        world.listB[0] == &templateNode &&
        templateNode.listCountA == 1 &&
        templateNode.listA != 0 &&
        templateNode.listA[0] == &world &&
        templateNode.gridCol == -1 &&
        templateNode.gridRow == -1;

    std::free(world.listB);
    std::free(templateNode.listA);
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabledPtr;
    return ok ? 0 : 1;
}

extern "C" int player_cache_gun_hardpoints_and_detach_displays_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_Object3DDataPartial gunData = {};
    zClass_Object3DDataPartial centerData = {};
    zClass_Object3DDataPartial leftData = {};
    zClass_Object3DDataPartial rightData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial gunNode = {};
    zClass_NodePartial centerNode = {};
    zClass_NodePartial leftNode = {};
    zClass_NodePartial rightNode = {};
    zClass_NodePartial *rootChildren[] = {&gunNode};
    zClass_NodePartial *gunChildren[] = {&centerNode, &leftNode, &rightNode};

    std::strcpy(rootNode.name, "root");
    std::strcpy(gunNode.name, "gun");
    std::strcpy(centerNode.name, "fpnt_c");
    std::strcpy(leftNode.name, "fpnt_l");
    std::strcpy(rightNode.name, "fpnt_r");

    rootNode.listCountB = 1;
    rootNode.listB = rootChildren;
    gunNode.listCountB = 3;
    gunNode.listB = gunChildren;

    gunNode.classId = 5;
    centerNode.classId = 5;
    leftNode.classId = 5;
    rightNode.classId = 5;
    gunNode.classData = &gunData;
    centerNode.classData = &centerData;
    leftNode.classData = &leftData;
    rightNode.classData = &rightData;

    const zMat4x3 gunMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                               0.0f, 0.0f, 1.0f, 11.0f, 12.0f, 13.0f};
    const zMat4x3 centerMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 21.0f, 22.0f, 23.0f};
    const zMat4x3 leftMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 31.0f, 32.0f, 33.0f};
    const zMat4x3 rightMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                 0.0f, 0.0f, 1.0f, 41.0f, 42.0f, 43.0f};
    SetObjectLocalMatrix(&gunData, gunMatrix);
    SetObjectLocalMatrix(&centerData, centerMatrix);
    SetObjectLocalMatrix(&leftData, leftMatrix);
    SetObjectLocalMatrix(&rightData, rightMatrix);

    playerState.rootNode = &rootNode;
    centerNode.userDataOrDiRef = 77;
    leftNode.userDataOrDiRef = 88;
    rightNode.userDataOrDiRef = 99;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    if (playerState.gunNode != &gunNode ||
        !Vec3Equals(playerState.gunNodeMatrixPos, {11.0f, 12.0f, 13.0f}) ||
        !Vec3Equals(playerState.firePointCenter, {21.0f, 22.0f, 23.0f}) ||
        !Vec3Equals(playerState.firePointLeft, {31.0f, 32.0f, 33.0f}) ||
        !Vec3Equals(playerState.firePointRight, {41.0f, 42.0f, 43.0f}) ||
        centerNode.userDataOrDiRef != 77 ||
        leftNode.userDataOrDiRef != 88 ||
        rightNode.userDataOrDiRef != 99) {
        return 1;
    }

    centerNode.userDataOrDiRef = 0;
    leftNode.userDataOrDiRef = 0;
    rightNode.userDataOrDiRef = 0;
    centerNode.flags = 1;
    leftNode.flags = 1;
    rightNode.flags = 1;

    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 1);
    if (centerNode.userDataOrDiRef != 0 ||
        leftNode.userDataOrDiRef != 0 ||
        rightNode.userDataOrDiRef != 0 ||
        (centerNode.flags & 0x200) != 0 ||
        (leftNode.flags & 0x200) != 0 ||
        (rightNode.flags & 0x200) != 0) {
        return 2;
    }

    zClass_NodePartial emptyRoot = {};
    std::strcpy(emptyRoot.name, "root");
    playerState.rootNode = &emptyRoot;
    playerState.gunNode = &gunNode;
    Player::CacheGunHardpointsAndDetachDisplays(&saveState, 0);
    return playerState.gunNode == 0 ? 0 : 3;
}

extern "C" int player_load_weapon_banks_and_select_defaults_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;

    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_HudSensorTracker.missionId = 8;

    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakePlayerZbdManager(sentinel);
    g_zUtil_ZbdManager = &manager;

    zClass_NodePartial rootNode = {};
    std::strcpy(rootNode.name, "local_vehicle");

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    PlayerMasterCommonData commonData = {};
    PlayerMasterWeaponSpec specA = {};
    PlayerMasterWeaponSpec specB = {};
    saveState.playerState = &playerState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    playerState.lifecycleState = 1;

    commonData.weaponSpecHead = &specA;
    commonData.weaponSpecTail = &specB;
    commonData.weaponSpecCount = 2;
    commonData.weaponNodeCount = 2;
    specA.next = &specB;

    std::strcpy(specA.optCatalogName, "WEP_2_0");
    specA.missionRequirementOrGateId = 7;
    specA.mountLayoutFlags = 0;
    specA.startAmmoOrCharge = 12.5f;
    specA.dispatchRepeatDelay = 1.25f;
    specA.aiAttackRangeMin = 2.5f;
    specA.aiAttackRangeMax = 9.5f;
    specA.fireSlotRecoilFlags = 1;
    specA.initialHardpointSelectState = 2;

    std::strcpy(specB.optCatalogName, "WEP_1_1");
    specB.missionRequirementOrGateId = 9;
    specB.startAmmoOrCharge = 33.0f;
    specB.dispatchRepeatDelay = 4.0f;
    specB.aiAttackRangeMin = 5.0f;
    specB.aiAttackRangeMax = 6.0f;

    OptCatalogEntryDef entries[2] = {};
    entries[0].keyName = const_cast<char *>("WEP_2_0");
    entries[0].displayName = const_cast<char *>("MountA");
    entries[1].keyName = const_cast<char *>("WEP_1_1");
    entries[1].displayName = const_cast<char *>("MountB");
    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 2;

    PlayerGunFireController &oldController = playerState.altWeaponBanks[5].controllerB;
    oldController.flags = 4;
    oldController.ammoOrCharge = 99.0f;
    oldController.attachNodePrimary =
        reinterpret_cast<zClass_NodePartial *>(static_cast<std::uintptr_t>(1));
    oldController.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    if (oldController.trailRuntimeState == 0) {
        return 7;
    }
    playerState.pendingAltCameraToggle = 1;
    playerState.timedHitStatus.runtimeFlags = 3;
    playerState.timedHitStatus.currentLevel = 0.5f;
    playerState.timedHitStatus.targetLevel = 1.0f;

    Player::LoadWeaponBanksAndSelectDefaults(&saveState);

    PlayerGunFireController &availableController = playerState.altWeaponBanks[2].controllerA;
    PlayerGunFireController &lockedController = playerState.altWeaponBanks[1].controllerB;
    zZbdSectionHandlerNode *const minesNode = sentinel.next;

    const bool resetOk =
        oldController.weaponBankIndex == 5 &&
        oldController.weaponSideIndex == 1 &&
        (oldController.flags & 4) == 0 &&
        oldController.ammoOrCharge == 0.0f &&
        oldController.attachNodePrimary == 0 &&
        oldController.trailRuntimeState == 0;
    const bool availableOk =
        availableController.optCatalogEntry == &entries[0] &&
        (availableController.flags & 4) != 0 &&
        (availableController.flags & 1) != 0 &&
        availableController.ammoOrCharge == 12.5f &&
        availableController.dispatchRepeatDelay == 1.25f &&
        availableController.aiAttackRangeMin == 2.5f &&
        availableController.aiAttackRangeMax == 9.5f &&
        availableController.initialHardpointSelectState == 2;
    const bool lockedOk =
        lockedController.optCatalogEntry == &entries[1] &&
        (lockedController.flags & 4) == 0 &&
        lockedController.ammoOrCharge == 0.0f;
    const bool selectionOk =
        playerState.activeAltGunController == &availableController &&
        playerState.activePrimaryGunController == &playerState.altWeaponBanks[1].controllerA &&
        playerState.altHardpointSelectState == 2 &&
        playerState.cachedAltSelectionCode == 200 &&
        playerState.cachedPrimarySelectionCode == 100 &&
        playerState.primaryHardpointSelectState == 2;
    const bool finalStateOk =
        playerState.pendingAltCameraToggle == 0 &&
        playerState.timedHitStatus.runtimeFlags == 0 &&
        playerState.timedHitStatus.currentLevel == 0.0f &&
        playerState.timedHitStatus.targetLevel == 0.0f &&
        playerState.timedHitStatus.lightParentNode == &rootNode;
    const bool zbdOk =
        minesNode != &sentinel &&
        minesNode->sectionHandler.sectionName != 0 &&
        std::strcmp(minesNode->sectionHandler.sectionName, "Mines") == 0 &&
        minesNode->sectionHandler.onPreLoad != 0 &&
        minesNode->sectionHandler.onDataReady != 0 &&
        minesNode->sectionHandler.sortOrder == 1000 &&
        minesNode->sectionHandler.userData == 0;

    ClearPlayerRegisteredHandlers(sentinel);
    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_zUtil_ZbdManager = oldZbdManager;

    if (!resetOk) {
        return 1;
    }
    if (!availableOk) {
        return 2;
    }
    if (!lockedOk) {
        return 3;
    }
    if (!selectionOk) {
        return 4;
    }
    if (!finalStateOk) {
        return 5;
    }
    return zbdOk ? 0 : 6;
}

extern "C" int player_free_alt_weapon_trail_runtime_states_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    playerState.altWeaponBanks[0].controllerA.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(1));
    playerState.altWeaponBanks[0].controllerB.trailRuntimeState =
        reinterpret_cast<OptCatalogTrailRuntimeState *>(static_cast<std::uintptr_t>(2));
    playerState.altWeaponBanks[1].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[4].controllerB.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));
    playerState.altWeaponBanks[9].controllerA.trailRuntimeState =
        static_cast<OptCatalogTrailRuntimeState *>(std::malloc(8));

    if (playerState.altWeaponBanks[1].controllerA.trailRuntimeState == 0 ||
        playerState.altWeaponBanks[4].controllerB.trailRuntimeState == 0 ||
        playerState.altWeaponBanks[9].controllerA.trailRuntimeState == 0) {
        return 1;
    }

    Player::FreeAltWeaponTrailRuntimeStates(&saveState);
    return playerState.altWeaponBanks[0].controllerA.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(1)) &&
                   playerState.altWeaponBanks[0].controllerB.trailRuntimeState ==
                       reinterpret_cast<OptCatalogTrailRuntimeState *>(
                           static_cast<std::uintptr_t>(2))
               ? 0
               : 2;
}

extern "C" int player_check_mission_weapon_availability_smoke(void) {
    const int oldMissionId = g_HudSensorTracker.missionId;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    int available = -1;
    g_HudSensorTracker.missionId = 8;
    Player::CheckMissionWeaponAvailability(0, 7, 0x61, &available);
    const bool singlePlayerUnlocked = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 9, 0x61, &available);
    const bool singlePlayerLockedByThreshold = available == 0;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x10, &available);
    const bool singlePlayerZeroThresholdLocked = available == 0;

    networkEnabled = 1;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x61, &available);
    const bool networkMission8LaserSabre = available == 1;

    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x31, &available);
    const bool networkMission8NapalmLocked = available == 0;

    g_HudSensorTracker.missionId = 11;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x80, &available);
    const bool networkMission11Missile = available == 1;

    g_HudSensorTracker.missionId = 5;
    available = -1;
    Player::CheckMissionWeaponAvailability(0, 0, 0x10, &available);
    const bool earlyNetworkMissionLocked = available == 0;

    g_HudSensorTracker.missionId = oldMissionId;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    return singlePlayerUnlocked && singlePlayerLockedByThreshold &&
                   singlePlayerZeroThresholdLocked && networkMission8LaserSabre &&
                   networkMission8NapalmLocked && networkMission11Missile &&
                   earlyNetworkMissionLocked
               ? 0
               : 1;
}

extern "C" int player_apply_primary_weapon_switch_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerGunFireController previousController = {};
    PlayerGunFireController newController = {};
    zClass_NodePartial previousPrimary = {};
    zClass_NodePartial previousSecondary = {};
    zClass_NodePartial newPrimary = {};
    zClass_NodePartial newSecondary = {};

    saveState.playerState = &playerState;
    previousController.attachNodePrimary = &previousPrimary;
    previousController.attachNodeSecondary = &previousSecondary;
    newController.attachNodePrimary = &newPrimary;
    newController.attachNodeSecondary = &newSecondary;
    newController.weaponBankIndex = 6;
    newController.weaponSideIndex = 1;

    previousPrimary.classId = 5;
    previousSecondary.classId = 5;
    newPrimary.classId = 5;
    newSecondary.classId = 5;
    previousPrimary.flags = 0x04;
    previousSecondary.flags = 0x04;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &previousController, &newController);
    const bool firstSwitchOk =
        playerState.activePrimaryGunController == &newController &&
        playerState.primaryHardpointSelectState == 2 &&
        playerState.cachedPrimarySelectionCode == 601 &&
        (previousPrimary.flags & 0x04) == 0 && (previousSecondary.flags & 0x04) == 0 &&
        (newPrimary.flags & 0x04) != 0 && (newSecondary.flags & 0x04) != 0;
    if (!firstSwitchOk) {
        return 1;
    }

    PlayerGunFireController nullSecondaryPrevious = {};
    PlayerGunFireController nextController = {};
    zClass_NodePartial nullPreviousPrimary = {};
    zClass_NodePartial nextPrimary = {};
    nullPreviousPrimary.classId = 5;
    nullPreviousPrimary.flags = 0x04;
    nextPrimary.classId = 5;
    nullSecondaryPrevious.attachNodePrimary = &nullPreviousPrimary;
    nextController.attachNodePrimary = &nextPrimary;
    nextController.weaponBankIndex = 2;
    nextController.weaponSideIndex = 0;

    Player::ApplyPrimaryWeaponSwitch(&saveState, &nullSecondaryPrevious, &nextController);
    return playerState.activePrimaryGunController == &nextController &&
                   playerState.cachedPrimarySelectionCode == 200 &&
                   (nullPreviousPrimary.flags & 0x04) == 0 && (nextPrimary.flags & 0x04) != 0
               ? 0
               : 2;
}

extern "C" int player_start_master_type_loop_sfx_handle_smoke(void) {
    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.markerBaseTime = 12.0f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[3] = &sample;
    zUtil_PlayerStateStorage playerState = {};
    playerState.masterCommonData = &commonData;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    zSndPlayHandle *const handle = saveState.StartMasterTypeLoopSfxHandle(3, 0.25f);

    g_zSnd_GlobalVolumeScalePtr = nullptr;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    if (handle == nullptr || handle != &sample.primaryVoice) {
        return 1;
    }
    if (playerState.modeLoopSfxHandle[3] != handle || sample.primaryVoice.ownerSample != &sample) {
        return 2;
    }

    return sample.markerBaseTime == 0.0f ? 0 : 3;
}

extern "C" int player_cache_disable_copter_snd_nodes_smoke(void) {
    zClass_TypeListLink *const oldHead = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldTail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    zClass_NodePartial copter1 = {};
    zClass_NodePartial copter2 = {};
    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    zClass_NodePartial *copter1Children[2] = {&healthy1, &snd1};
    zClass_NodePartial *copter2Children[2] = {&healthy2, &snd2};
    zClass_SoundDataPartial snd1Data = {};
    zClass_SoundDataPartial snd2Data = {};
    zClass_TypeListLink copter1Link = {&copter1, nullptr, nullptr, 0};
    zClass_TypeListLink copter2Link = {&copter2, &copter1Link, nullptr, 0};
    copter1Link.next = &copter2Link;

    std::strcpy(copter1.name, "copter01");
    std::strcpy(copter2.name, "copter02");
    std::strcpy(healthy1.name, "healthy");
    std::strcpy(healthy2.name, "healthy");
    std::strcpy(snd1.name, "snd_chopper");
    std::strcpy(snd2.name, "snd_chopper");
    copter1.listCountB = 2;
    copter1.listB = copter1Children;
    copter2.listCountB = 2;
    copter2.listB = copter2Children;
    snd1.classId = 10;
    snd1.flags = 0x04;
    snd1.classData = &snd1Data;
    snd2.classId = 10;
    snd2.flags = 0x04;
    snd2.classData = &snd2Data;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Stop = &TestDirectSoundStop;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};
    zSndSample sample = {};
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    zClass_TypeList::Head(6) = &copter1Link;
    zClass_TypeList::Tail(6) = &copter2Link;
    g_Player_CopterHealthyNode1 = nullptr;
    g_Player_CopterHealthyNode2 = nullptr;
    g_Player_CopterSndNode1 = nullptr;
    g_Player_CopterSndNode2 = nullptr;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerBootstrapTestStopCount = 0;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool lazyLookupOk =
        g_Player_CopterHealthyNode1 == &healthy1 && g_Player_CopterHealthyNode2 == &healthy2 &&
        g_Player_CopterSndNode1 == &snd1 && g_Player_CopterSndNode2 == &snd2 &&
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 &&
        g_PlayerBootstrapTestStopCount == 1;

    snd1.flags = 0x04;
    snd2.flags = 0x04;
    zClass_TypeList::Head(6) = nullptr;
    zClass_TypeList::Tail(6) = nullptr;

    Player::CacheDisableCopterSndNodesAndStopSample();
    const bool cachedPathOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) == 0 &&
        g_PlayerBootstrapTestStopCount == 2;

    zClass_TypeList::Head(6) = oldHead;
    zClass_TypeList::Tail(6) = oldTail;
    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return lazyLookupOk && cachedPathOk ? 0 : 1;
}

extern "C" int player_reactivate_copter_snd_nodes_if_healthy_smoke(void) {
    zClass_NodePartial *const oldHealthy1 = g_Player_CopterHealthyNode1;
    zClass_NodePartial *const oldHealthy2 = g_Player_CopterHealthyNode2;
    zClass_NodePartial *const oldSnd1 = g_Player_CopterSndNode1;
    zClass_NodePartial *const oldSnd2 = g_Player_CopterSndNode2;
    zSndSample *const oldSample = g_Player_CopterSndSample;
    const int oldBackend = g_zSnd_ActiveBackend;

    TestDirectSoundBufferVTable vtable = {};
    vtable.Play = &TestDirectSoundPlay;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    zSndPlayHandle handle1 = {};
    handle1.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zSndPlayHandle handle2 = {};
    handle2.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);
    zClass_SoundDataPartial sndData1 = {};
    sndData1.playHandle = &handle1;
    zClass_SoundDataPartial sndData2 = {};
    sndData2.playHandle = &handle2;

    zClass_NodePartial healthy1 = {};
    zClass_NodePartial healthy2 = {};
    zClass_NodePartial snd1 = {};
    zClass_NodePartial snd2 = {};
    healthy1.flags = 0x04;
    healthy2.flags = 0x04;
    snd1.classId = 10;
    snd1.classData = &sndData1;
    snd2.classId = 10;
    snd2.classData = &sndData2;

    g_Player_CopterHealthyNode1 = &healthy1;
    g_Player_CopterHealthyNode2 = &healthy2;
    g_Player_CopterSndNode1 = &snd1;
    g_Player_CopterSndNode2 = &snd2;
    g_Player_CopterSndSample = &sample;
    g_zSnd_ActiveBackend = 0;
    g_PlayerBootstrapTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool activePlaybackOk =
        (snd1.flags & 0x04) != 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerBootstrapTestPlayCount == 2;

    healthy1.flags = 0;
    snd1.flags = 0;
    snd2.flags = 0;
    sndData2.playHandle = nullptr;
    g_PlayerBootstrapTestPlayCount = 0;

    Player::ReactivateCopterSndNodesIfHealthy();
    const bool inactiveAndMissingHandleOk =
        (snd1.flags & 0x04) == 0 && (snd2.flags & 0x04) != 0 &&
        g_PlayerBootstrapTestPlayCount == 0;

    g_Player_CopterHealthyNode1 = oldHealthy1;
    g_Player_CopterHealthyNode2 = oldHealthy2;
    g_Player_CopterSndNode1 = oldSnd1;
    g_Player_CopterSndNode2 = oldSnd2;
    g_Player_CopterSndSample = oldSample;
    g_zSnd_ActiveBackend = oldBackend;

    return activePlaybackOk && inactiveAndMissingHandleOk ? 0 : 1;
}

extern "C" int player_apply_alt_weapon_switch_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    void *const oldVolumeScalePtr = g_zSnd_GlobalVolumeScalePtr;
    const int oldInitialized = g_zSnd_IsInitialized;
    const int oldPreInitialized = g_zSnd_PreInitialized;
    const int oldBackend = g_zSnd_ActiveBackend;
    const int oldMuteDepth = g_zSnd_MuteDepth;
    const int oldPlaybackEnabled = g_zSnd_Flag10PlaybackEnabled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    PlayerGunFireController initialController = {};
    initialController.weaponBankIndex = 2;
    initialController.weaponSideIndex = 1;
    playerState.altGunTransitionTimerA = 3.0f;
    playerState.altGunTransitionTimerB = 4.0f;

    Player::ApplyAltWeaponSwitch(&saveState, 0, &initialController);
    const bool initialOk =
        playerState.activeAltGunController == &initialController &&
        playerState.activeAltBankIndex == 2 &&
        playerState.altWeaponBanks[2].selectedSide == 1 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 16 &&
        playerState.altGunTransitionController == &initialController &&
        playerState.cachedAltSelectionCode == 201;
    if (!initialOk) {
        return 1;
    }

    TestDirectSoundBufferVTable vtable = {};
    vtable.GetStatus = &TestDirectSoundGetStatus;
    vtable.Play = &TestDirectSoundPlay;
    vtable.SetCurrentPosition = &TestDirectSoundSetInt;
    vtable.SetVolume = &TestDirectSoundSetInt;
    TestDirectSoundBuffer directSoundBuffer = {&vtable};

    float globalVolume = 1.0f;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    g_zSnd_IsInitialized = 1;
    g_zSnd_PreInitialized = 1;
    g_zSnd_ActiveBackend = 0;
    g_zSnd_MuteDepth = 0;
    g_zSnd_Flag10PlaybackEnabled = 1;
    g_PlayerBootstrapTestPlayCount = 0;

    zSndSample sample = {};
    sample.replayFields.flags = 8;
    sample.replayFields.gain = 0.5f;
    sample.primaryVoice.backendBuffer = reinterpret_cast<zSndBuffer *>(&directSoundBuffer);

    PlayerMasterCommonData commonData = {};
    commonData.sfxWeaponUp[0] = &sample;
    playerState.masterCommonData = &commonData;

    PlayerGunFireController previousController = {};
    PlayerGunFireController nextController = {};
    previousController.weaponBankIndex = 2;
    previousController.weaponSideIndex = 1;
    nextController.weaponBankIndex = 4;
    nextController.weaponSideIndex = 0;

    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState runtime = {};
    zClass_NodePartial trailNode = {};
    owner.activeTrailRuntime = &runtime;
    runtime.ownerEntry = &owner;
    runtime.activeNodeSlotCount = 1;
    runtime.activeNodeSlotCursor = 1;
    runtime.activeNodeSlots[0].node = &trailNode;
    trailNode.classId = 5;
    trailNode.flags = 0x04;
    previousController.trailRuntimeState = &runtime;

    playerState.altGunFireHeldFlag = 1;
    playerState.altGunTransitionTimerA = 5.0f;
    playerState.altGunTransitionTimerB = 6.0f;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    Player::ApplyAltWeaponSwitch(&saveState, &previousController, &nextController);

    const bool switchOk =
        playerState.activeAltGunController == &nextController &&
        playerState.activeAltBankIndex == 4 &&
        playerState.altWeaponBanks[4].selectedSide == 0 &&
        playerState.altHardpointSelectState == 0 &&
        playerState.altGunTransitionTimerA == 0.0f &&
        playerState.altGunTransitionTimerB == 0.0f &&
        playerState.altGunTransitionState == 4 &&
        playerState.altGunTransitionController == &previousController &&
        playerState.altGunFireHeldFlag == 0 &&
        playerState.cachedAltSelectionCode == 400 &&
        playerState.modeLoopSfxHandle[0] == &sample.primaryVoice &&
        sample.primaryVoice.ownerSample == &sample &&
        g_PlayerBootstrapTestPlayCount == 1 &&
        owner.activeTrailRuntime == 0 &&
        runtime.activeNodeSlotCursor == 0 &&
        (trailNode.flags & 0x04) == 0;

    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zSnd_GlobalVolumeScalePtr = oldVolumeScalePtr;
    g_zSnd_IsInitialized = oldInitialized;
    g_zSnd_PreInitialized = oldPreInitialized;
    g_zSnd_ActiveBackend = oldBackend;
    g_zSnd_MuteDepth = oldMuteDepth;
    g_zSnd_Flag10PlaybackEnabled = oldPlaybackEnabled;

    return switchOk ? 0 : 2;
}

extern "C" int player_apply_aim_pitch_to_direction_smoke(void) {
    zVec3 direction = {3.0f, 9.0f, 4.0f};
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    const float scale = PlayerFastSqrtEstimateForTest(1.0f / 25.0f);

    int failed = 0;
    failed |= !FloatNear(direction.x, 3.0f * scale);
    failed |= direction.y != 0.0f;
    failed |= !FloatNear(direction.z, 4.0f * scale);

    direction = zVec3_Make(0.0f, 12.0f, 0.0f);
    Player::ApplyAimPitchToDirection(&direction, 0.0f);
    failed |= !Vec3Equals(direction, zVec3_Make(0.0f, 0.0f, -1.0f));

    direction = zVec3_Make(0.0f, 0.0f, 0.0f);
    Player::ApplyAimPitchToDirection(&direction, 0.5f);
    const float diagonal = PlayerFastSqrtEstimateForTest((1.0f - 0.25f) * 0.5f);
    failed |= !FloatNear(direction.x, diagonal);
    failed |= direction.y != 0.5f;
    failed |= !FloatNear(direction.z, diagonal);

    return failed != 0 ? 1 : 0;
}

extern "C" int player_write_mines_zar_section_smoke(void) {
    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    GetTempPathA(sizeof(tempPath), tempPath);
    GetTempFileNameA(tempPath, "pmn", 0, tempFile);

    HANDLE const file =
        CreateFileA(tempFile, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    zZbdManager manager = {};
    manager.indexArchive.hFile = file;

    zZbdSectionHandler handler = {};
    handler.sectionName = "Mines";
    zZbdSectionCallbackCtx callbackCtx = {};
    callbackCtx.manager = &manager;
    callbackCtx.sectionHandler = &handler;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;

    OptCatalogEntryDef ignoredEntry = {};
    ignoredEntry.keyName = const_cast<char *>("ignored");
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    OptCatalogEntryDef entry = {};
    entry.keyName = const_cast<char *>("P_HEMINE");
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &entry;

    zClass_NodePartial ownerNode = {};
    std::strcpy(ownerNode.name, "mine_node");
    zClass_NodePartial projectileNode = {};
    projectileNode.classId = 5;
    zClass_Object3DDataPartial projectileData = {};
    projectileData.scale = {2.0f, 3.0f, 4.0f};
    projectileNode.classData = &projectileData;

    OptCatalogRuntimeInstanceStorage runtime = {};
    runtime.ownerNode = &ownerNode;
    runtime.projectileNode = &projectileNode;
    runtime.pos = {5.0f, 6.0f, 7.0f};
    entry.activeRuntimeListHead = &runtime;

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    const int result = Player::WriteMinesZarSection(&callbackCtx, 0);

    PlayerMineSaveEntry dummy = {};
    PlayerMineSaveEntry mine = {};
    DWORD readDummy = 0;
    DWORD readMine = 0;
    SetFilePointer(file, 0, 0, FILE_BEGIN);
    ReadFile(file, &dummy, 0x60, &readDummy, 0);
    ReadFile(file, &mine, 0x60, &readMine, 0);

    const bool ok =
        result == 1 && manager.indexArchive.recordCount == 2 &&
        manager.indexArchive.records != 0 &&
        std::strcmp(manager.indexArchive.records[0].name, "Mines/DummyMineData") == 0 &&
        std::strcmp(manager.indexArchive.records[1].name, "Mines/MineData000") == 0 &&
        readDummy == 0x60 && readMine == 0x60 && dummy.resetMarker == 1 &&
        std::strcmp(dummy.ownerNodeName, "Dummy") == 0 && mine.resetMarker == 0 &&
        std::strncmp(mine.optCatalogName, "P_HEMINE", 0x20) == 0 &&
        Vec3Equals(mine.spawnPos, runtime.pos) && Vec3Equals(mine.scale, projectileData.scale) &&
        std::strncmp(mine.ownerNodeName, "mine_node", 0x20) == 0;

    g_GameStateOrMapTable = oldGameState;
    std::free(manager.indexArchive.records);
    manager.indexArchive.records = 0;
    CloseHandle(file);
    return ok ? 0 : 2;
}

extern "C" int player_mines_zar_read_entry_or_reset_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);

    int result = 0;

    zUtil_PlayerStateStorage playerState = {};
    zUtil_SaveGameState saveState = {};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef resetEntryA = {};
    OptCatalogEntryDef resetEntryB = {};
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &resetEntryA;
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &resetEntryB;

    OptCatalogRuntimeInstanceStorage resetRuntimeA = {};
    OptCatalogRuntimeInstanceStorage resetRuntimeB = {};
    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    zClass_NodeFreeListSlot resetProjectileA = {};
    zClass_NodeFreeListSlot resetProjectileB = {};
    zClass_Object3DDataPartial resetProjectileDataA = {};
    zClass_Object3DDataPartial resetProjectileDataB = {};
    zClass_NodePartial resetRuntimeWorld = {};
    resetRuntimeWorld.classId = 3;
    resetRuntimeWorld.flags = 1;
    resetProjectileA.node.classId = 5;
    resetProjectileA.node.flags = 1;
    resetProjectileA.node.classData = &resetProjectileDataA;
    resetProjectileA.damageHandler = &resetEntryA;
    resetProjectileB.node.classId = 5;
    resetProjectileB.node.flags = 1;
    resetProjectileB.node.classData = &resetProjectileDataB;
    resetProjectileB.damageHandler = &resetEntryB;
    resetRuntimeA.projectileNode = &resetProjectileA.node;
    resetRuntimeA.lifetime = 4.0f;
    resetRuntimeB.projectileNode = &resetProjectileB.node;
    resetRuntimeB.lifetime = 5.0f;
    resetEntryA.activeRuntimeListHead = &resetRuntimeA;
    resetEntryB.activeRuntimeListHead = &resetRuntimeB;
    g_OptCatalogRuntimeWorld = &resetRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileA.node);
    zClass_Class::AddChild(&resetRuntimeWorld, &resetProjectileB.node);

    PlayerMineSaveEntry resetData = {};
    resetData.resetMarker = 1;
    Player::Mines_ZAR_ReadEntryOrReset(0, 0, &resetData, sizeof(resetData), 0);

    OptCatalogEntryDef spawnEntry = {};
    OptCatalogRuntimeInstanceStorage spawnRuntime = {};
    zClass_NodeFreeListSlot spawnProjectile = {};
    zClass_Object3DDataPartial spawnProjectileData = {};
    zClass_NodePartial spawnRuntimeWorld = {};
    zClass_NodePartial ownerNode = {};
    zClass_TypeListLink ownerLink = {&ownerNode, 0, 0, 0};
    PlayerMineSaveEntry mineData = {};

    if (ignoredEntry.activeRuntimeListHead != 0 ||
        resetEntryA.activeRuntimeListHead != 0 ||
        resetEntryB.activeRuntimeListHead != 0 || resetRuntimeWorld.listCountB != 0 ||
        resetProjectileA.node.listCountA != 0 || resetProjectileB.node.listCountA != 0 ||
        g_OptCatalogFreeRuntimeInstanceList != &resetRuntimeB ||
        resetRuntimeB.next != &resetRuntimeA || resetRuntimeA.next != &freeSentinel ||
        resetProjectileA.damageHandler != 0 || resetProjectileB.damageHandler != 0 ||
        resetProjectileDataA.scale.x != 1.0f || resetProjectileDataB.scale.x != 1.0f) {
        result = 1;
        goto restore;
    }

    spawnEntry.keyName = const_cast<char *>("P_HEMINE");
    spawnEntry.flyoutHealth = 8.0f;
    g_OptCatalog_EntryTable = &spawnEntry;
    g_OptCatalog_EntryCount = 1;

    spawnRuntimeWorld.classId = 3;
    spawnRuntimeWorld.flags = 1;
    spawnProjectile.node.classId = 5;
    spawnProjectile.node.flags = 1;
    spawnProjectile.node.classData = &spawnProjectileData;
    spawnRuntime.projectileNode = &spawnProjectile.node;
    std::strcpy(ownerNode.name, "mine_owner");
    zClass_TypeList::Head(6) = &ownerLink;
    zClass_TypeList::Tail(6) = &ownerLink;
    g_OptCatalogRuntimeWorld = &spawnRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &spawnRuntime;
    g_OptCatalogNextSpawnScale = 3.0f;

    std::strcpy(mineData.optCatalogName, "P_HEMINE");
    mineData.spawnPos = {10.0f, 11.0f, 12.0f};
    mineData.scale = {2.0f, 3.0f, 4.0f};
    std::strcpy(mineData.ownerNodeName, "mine_owner");
    Player::Mines_ZAR_ReadEntryOrReset(0, 0, &mineData, sizeof(mineData), 0);

    if (spawnEntry.activeRuntimeListHead != &spawnRuntime ||
        g_OptCatalogFreeRuntimeInstanceList != 0 || spawnRuntime.ownerNode != &ownerNode ||
        !Vec3Equals(spawnRuntime.pos, mineData.spawnPos) || spawnRuntime.spawnScale != 3.0f ||
        g_OptCatalogNextSpawnScale != 1.0f || spawnRuntimeWorld.listCountB != 1 ||
        spawnRuntimeWorld.listB[0] != &spawnProjectile.node ||
        spawnProjectile.node.callbackContext !=
            reinterpret_cast<zClass_NodePartial *>(&spawnRuntime) ||
        spawnProjectileData.scale.x != 2.0f || spawnProjectileData.scale.y != 3.0f ||
        spawnProjectileData.scale.z != 4.0f) {
        result = 2;
        goto restore;
    }

restore:
    g_GameStateOrMapTable = oldGameState;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    return result;
}

extern "C" int player_remove_all_deployed_mines_smoke(void) {
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    OptCatalogRuntimeInstanceStorage *const oldFreeRuntimeList =
        g_OptCatalogFreeRuntimeInstanceList;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;

    zClass_NodePartial rootNode = {};
    playerState.rootNode = &rootNode;

    OptCatalogEntryDef ignoredEntry = {};
    OptCatalogEntryDef mineEntries[4] = {};
    OptCatalogRuntimeInstanceStorage ignoredRuntime = {};
    OptCatalogRuntimeInstanceStorage mineRuntimes[4] = {};
    zClass_NodeFreeListSlot ignoredProjectile = {};
    zClass_NodeFreeListSlot mineProjectiles[4] = {};
    zClass_Object3DDataPartial ignoredData = {};
    zClass_Object3DDataPartial mineData[4] = {};
    zClass_NodePartial runtimeWorld = {};
    zClass_NodePartial *worldChildren[5] = {
        &ignoredProjectile.node,
        &mineProjectiles[0].node,
        &mineProjectiles[1].node,
        &mineProjectiles[2].node,
        &mineProjectiles[3].node,
    };

    runtimeWorld.classId = 3;
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 5;
    ignoredProjectile.node.classId = 5;
    ignoredProjectile.node.classData = &ignoredData;
    ignoredRuntime.ownerNode = &rootNode;
    ignoredRuntime.projectileNode = &ignoredProjectile.node;
    ignoredRuntime.lifetime = 0.0f;
    ignoredEntry.activeRuntimeListHead = &ignoredRuntime;
    playerState.altWeaponBanks[3].controllerA.optCatalogEntry = &ignoredEntry;

    for (int index = 0; index < 4; ++index) {
        mineProjectiles[index].node.classId = 5;
        mineProjectiles[index].node.classData = &mineData[index];
        mineRuntimes[index].ownerNode = &rootNode;
        mineRuntimes[index].projectileNode = &mineProjectiles[index].node;
        mineRuntimes[index].lifetime = 0.0f;
        mineEntries[index].activeRuntimeListHead = &mineRuntimes[index];
    }

    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntries[0];
    playerState.altWeaponBanks[4].controllerB.optCatalogEntry = &mineEntries[1];
    playerState.altWeaponBanks[5].controllerA.optCatalogEntry = &mineEntries[2];
    playerState.altWeaponBanks[5].controllerB.optCatalogEntry = &mineEntries[3];

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    Player::RemoveAllDeployedMines(&saveState);

    const bool ignoredOk =
        ignoredEntry.activeRuntimeListHead == &ignoredRuntime &&
        ignoredRuntime.next == 0 &&
        ignoredProjectile.node.listCountA == 0;
    const bool minesCleared =
        mineEntries[0].activeRuntimeListHead == 0 &&
        mineEntries[1].activeRuntimeListHead == 0 &&
        mineEntries[2].activeRuntimeListHead == 0 &&
        mineEntries[3].activeRuntimeListHead == 0;
    const bool freeListOk =
        g_OptCatalogFreeRuntimeInstanceList == &mineRuntimes[3] &&
        mineRuntimes[3].next == &mineRuntimes[2] &&
        mineRuntimes[2].next == &mineRuntimes[1] &&
        mineRuntimes[1].next == &mineRuntimes[0] &&
        mineRuntimes[0].next == &freeSentinel;
    const bool worldOk =
        runtimeWorld.listCountB == 1 &&
        runtimeWorld.listB[0] == &ignoredProjectile.node;

    int failure = 0;
    if (!ignoredOk) {
        failure = 1;
    } else if (!minesCleared) {
        failure = 2;
    } else if (!freeListOk) {
        failure = 3;
    } else if (!worldOk) {
        failure = 4;
    }

    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    return failure;
}

extern "C" int player_init_state_from_name_and_master_common_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    int gameControlOptions = 0;
    int networkEnabled = 0;

    PlayerMasterCommonData commonData = {};
    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    commonData.cambackDist0 = 3.0f;
    commonData.cambackSide1 = 4.0f;
    commonData.cambackBase1 = 5.0f;
    commonData.cambackDist1 = 6.0f;
    commonData.cambackSide2 = 7.0f;
    commonData.cambackBase2 = 8.0f;
    commonData.cambackDist2 = 9.0f;
    commonData.aimYawRate = 10.0f;
    commonData.aimYawMax = 11.0f;
    commonData.maxHealth = 250.0f;

    zClass_Object3DDataPartial rootData = {};
    zClass_Object3DDataPartial targetData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial targetNode = {};
    zClass_NodePartial bodyNode = {};
    zClass_NodePartial turretNode = {};
    zClass_NodePartial doorLeftNode = {};
    zClass_NodePartial doorRightNode = {};
    zClass_NodePartial shadowNode = {};
    zClass_NodePartial *rootChildren[] = {&targetNode, &bodyNode, &turretNode,
                                          &doorLeftNode, &doorRightNode, &shadowNode};

    std::strcpy(rootNode.name, "tank");
    std::strcpy(targetNode.name, "target");
    std::strcpy(bodyNode.name, "body");
    std::strcpy(turretNode.name, "turret");
    std::strcpy(doorLeftNode.name, "doorleft");
    std::strcpy(doorRightNode.name, "doorright");
    std::strcpy(shadowNode.name, "shadow");
    rootNode.classId = 5;
    targetNode.classId = 5;
    rootNode.classData = &rootData;
    targetNode.classData = &targetData;
    rootNode.listCountB = 6;
    rootNode.listB = rootChildren;
    targetNode.flags = 0x04;
    const zMat4x3 rootMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 10.0f, 20.0f, 30.0f};
    const zMat4x3 targetMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 2.0f, 3.0f, 4.0f};
    SetObjectLocalMatrix(&rootData, rootMatrix);
    SetObjectLocalMatrix(&targetData, targetMatrix);

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 1;
    g_Player_NextOrdinal = 1;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = 0;
    g_AINetListTail = 0;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_EntryCount = 0;

    Player::InitStateFromNameAndMasterCommonData(&saveState, "tank",
                                                 "tank_common");

    int result = 0;
    if (playerState.masterCommonData != &commonData || playerState.playerOrdinal != 1 ||
        g_Player_NextOrdinal != 2 ||
        g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)(&saveState)) {
        result = 1;
    } else if (!Vec3Equals(playerState.worldPos, zVec3_Make(10.0f, 20.0f, 30.0f)) ||
               playerState.pitchPoseCache != 0.0f || playerState.yawPoseCache != 0.0f ||
               playerState.rollPoseCache != 0.0f ||
               !Vec3Equals(playerState.steerBasisRaw, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.steerBasisNorm, zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               !Vec3Equals(playerState.cameraDirFlat, zVec3_Make(0.0f, 0.0f, -1.0f))) {
        result = 2;
    } else if (playerState.lifecycleState != 4 ||
               playerState.thirdPersonSideOffset != 1.0f ||
               playerState.thirdPersonBaseYOffset != 2.0f ||
               playerState.cameraDistance != 3.0f ||
               playerState.cameraConfigParam0 != 4.0f ||
               playerState.cameraConfigParam5 != 9.0f ||
               playerState.cameraYOffset != 11.0f ||
               !Vec3Equals(playerState.cameraState2TargetOffset,
                           zVec3_Make(0.0f, 150.0f, 0.0f)) ||
               !Vec3Equals(playerState.altGunAimOrigin,
                           zVec3_Make(0.0f, 0.0f, -1.0f)) ||
               playerState.activeAltBankIndex != 1 || playerState.autoTurnActive != 0 ||
               playerState.cameraTransitionTimer != 0 ||
               playerState.cameraTransitionBlend != 1.0f) {
        result = 3;
    } else if (!Vec3Equals(playerState.fxOffsetLocal, zVec3_Make(2.0f, 3.0f, 4.0f)) ||
               !Vec3Equals(playerState.fxOffsetWorld, zVec3_Make(12.0f, 23.0f, 34.0f)) ||
               (targetNode.flags & 0x04) != 0 || playerState.bodyNode != &bodyNode ||
               playerState.turretNode != &turretNode ||
               playerState.doorLeftNode != &doorLeftNode ||
               playerState.doorRightNode != &doorRightNode ||
               playerState.modeVariantNode != &shadowNode || playerState.gunNode != 0 ||
               playerState.statusMeterValue != 250.0f ||
               playerState.statusMeterScaled != 1.0f ||
               playerState.damageProtectionActive != 0 ||
               playerState.queuedFixedDamageFlag != 0 ||
               playerState.recentHitValid != 0 ||
               playerState.recentHitLightHandle != 0 ||
               playerState.nanitePanelLevel != 0) {
        result = 4;
    }

    AINetNode aiNode = {};
    aiNode.position = zVec3_Make(9.0f, 20.0f, 31.0f);
    AINet aiNet = {};
    aiNet.netId = 77;
    aiNet.aiType = AINET_TYPE_FI;
    aiNet.activateRadius = 5.0f;
    aiNet.attackRadius = 6.0f;
    aiNet.attackDwell = 7.0f;
    aiNet.notPursuitDwell = 8.0f;
    aiNet.returnRange = 9.0f;
    aiNet.hideTime0 = 10.0f;
    aiNet.hideTime1 = 11.0f;
    aiNet.attackStrategy = AINET_STRAT_ZIG;
    aiNet.nodeListHead = &aiNode;

    zClass_NodePartial healthyNode = {};
    std::strcpy(healthyNode.name, "healthy");
    healthyNode.flags = 0x08;
    zClass_NodePartial *aiRootChildren[] = {&healthyNode};
    rootNode.listCountB = 1;
    rootNode.listB = aiRootChildren;
    targetNode.flags = 0x04;
    playerState = {};
    saveState = {};
    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.aiNetId = 77;
    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_Player_NextOrdinal = 2;
    g_GameStateOrMapTable = 0;
    g_Time_AccumulatedTimeSec = 42.0f;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = &aiNet;
    g_AINetListTail = &aiNet;

    if (result == 0) {
        Player::InitStateFromNameAndMasterCommonData(&saveState, "net_tank",
                                                     "tank_common");

        if (playerState.lifecycleState != 2 || playerState.aiNet != &aiNet ||
            playerState.aiTopLevelState != 3 ||
            playerState.aiCurrentSteeringSubstate != AINET_STRAT_ZIG ||
            playerState.aiHideTime0 != 10.0f || playerState.aiHideTime1 != 11.0f ||
            playerState.aiCurrentPathNode != &aiNode ||
            playerState.aiHomePathNode != &aiNode ||
            playerState.aiActivationRadiusSq != 25.0f ||
            playerState.aiAttackRadiusSq != 36.0f ||
            playerState.aiMode2AttackDwell != 7.0f ||
            playerState.aiNotPursuitDwell != 8.0f ||
            playerState.aiRestoreDistanceSq != 81.0f ||
            saveState.aiPeerRingNext != &saveState ||
            playerState.aiStateUntilTime != 52.0f ||
            playerState.aiStateStartTime != 52.0f ||
            (healthyNode.flags & 0x08) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_GameStateOrMapTable = oldGameState;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    return result;
}

extern "C" int player_bind_modal_state_from_master_modal_data_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterCommonData commonData = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData nonMatchingModalData = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial cacheNodes[12] = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[28] = {};

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(nonMatchingModalData.modalName, "tank_common");
    std::strcpy(nonMatchingModalData.modeName, "hover");
    nonMatchingModalData.next = &modalData;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    playerState.masterCommonData = &commonData;
    playerState.rootNode = &rootNode;
    std::strcpy(rootNode.name, "vehicle");
    rootNode.listCountB = 28;
    rootNode.listB = children;

    const char *const cacheNames[12] = {"right_morphs", "left_morphs", "chassis",
                                        "rtracks",      "ltracks",     "props",
                                        "caustic1",     "wake",        "splash_l",
                                        "splash_r",     "dust_l",      "dust_r"};
    for (int i = 0; i < 12; ++i) {
        std::strcpy(cacheNodes[i].name, cacheNames[i]);
        children[i] = &cacheNodes[i];
    }

    for (int i = 0; i < 4; ++i) {
        children[12 + i] = &supportNodes[i];
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(100 + i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    for (int i = 0; i < 12; ++i) {
        children[16 + i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    g_PlayerSaveStateListHead = &saveState;
    g_PlayerSaveStateListTail = &saveState;
    g_PlayerSaveStateListAux = 1;
    g_PlayerSaveStateCount = 1;
    g_PlayerMasterModalDataHead = &nonMatchingModalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 2;

    Player::BindModalStateFromMasterModalData(&saveState, &modalState, "tank_object",
                                              "track");

    int result = 0;
    if (modalState.masterModalData != &modalData ||
        modalState.nodeRightMorphs != &cacheNodes[0] ||
        modalState.nodeLeftMorphs != &cacheNodes[1] ||
        modalState.modalNode != &cacheNodes[2] ||
        modalState.nodeRTracks != &cacheNodes[3] ||
        modalState.nodeLTracks != &cacheNodes[4] || modalState.nodeProps != &cacheNodes[5] ||
        modalState.nodeCaustic1 != &cacheNodes[6] ||
        modalState.nodeWake != &cacheNodes[7] ||
        modalState.nodeSplashL != &cacheNodes[8] ||
        modalState.nodeSplashR != &cacheNodes[9] ||
        modalState.nodeDustL != &cacheNodes[10] ||
        modalState.nodeDustR != &cacheNodes[11]) {
        result = 1;
    }

    if (result == 0 &&
        (modalState.modalStateCode != 4 || modalState.chassisPitchFilterState != 0.0f ||
         modalState.chassisRollFilterState != 0.0f)) {
        result = 2;
    }

    for (int i = 0; result == 0 && i < 4; ++i) {
        const float base = static_cast<float>(100 + i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            result = 3;
        }
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    if (result == 0 && modalData.probePointCount != 12) {
        result = 4;
    }
    for (int i = 0; result == 0 && i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            result = 5;
        }
    }

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    return result;
}

extern "C" int player_build_support_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial supportNodes[4] = {};
    zClass_Object3DDataPartial supportData[4] = {};
    zClass_NodePartial *children[4] = {
        &supportNodes[0], &supportNodes[1], &supportNodes[2], &supportNodes[3]};
    modelNode.listCountB = 4;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 4; ++i) {
        std::sprintf(supportNodes[i].name, "support%02d", i);
        supportNodes[i].classId = 5;
        supportNodes[i].classData = &supportData[i];
        supportNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&supportData[i], matrix);
    }

    const int ok = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (ok != 1) {
        return 1;
    }

    for (int i = 0; i < 4; ++i) {
        const float base = static_cast<float>(i * 10);
        if (!Vec3Equals(modalData.probePoints[15 + i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (supportNodes[i].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[17] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modelNode.listCountB = 2;
    supportNodes[0].flags = 0x04;
    supportNodes[1].flags = 0x04;
    const int missing = Player::BuildSupportPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (supportNodes[0].flags & 0x04) != 0 ||
        (supportNodes[1].flags & 0x04) != 0 ||
        !Vec3Equals(modalData.probePoints[17], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_build_collision_points_from_model_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerMasterModalData modalData = {};
    PlayerModalState primaryModalState = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &primaryModalState;
    primaryModalState.masterModalData = &modalData;

    zClass_NodePartial modelNode = {};
    zClass_NodePartial collisionNodes[12] = {};
    zClass_Object3DDataPartial collisionData[12] = {};
    zClass_NodePartial *children[12] = {};
    modelNode.listCountB = 12;
    modelNode.listB = children;
    std::strcpy(modelNode.name, "vehicle");

    for (int i = 0; i < 12; ++i) {
        children[i] = &collisionNodes[i];
        std::sprintf(collisionNodes[i].name, "collide%02d", i);
        collisionNodes[i].classId = 5;
        collisionNodes[i].classData = &collisionData[i];
        collisionNodes[i].flags = 0x04;
        const float base = static_cast<float>(i * 10);
        const zMat4x3 matrix = {1.0f,      0.0f,      0.0f, 0.0f, 1.0f, 0.0f,
                                0.0f,      0.0f,      1.0f, base + 1.0f,
                                base + 2.0f, base + 3.0f};
        SetObjectLocalMatrix(&collisionData[i], matrix);
    }

    const int ok = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (ok != 1 || modalData.probePointCount != 12) {
        return 1;
    }

    const int expectedOrder[12] = {0, 1, 2, 6, 7, 8, 3, 4, 5, 9, 10, 11};
    for (int i = 0; i < 12; ++i) {
        const float base = static_cast<float>(expectedOrder[i] * 10);
        if (!Vec3Equals(modalData.probePoints[i],
                        zVec3_Make(base + 1.0f, base + 2.0f, base + 3.0f)) ||
            (collisionNodes[expectedOrder[i]].flags & 0x04) != 0) {
            return 2;
        }
    }

    modalData.probePoints[0] = zVec3_Make(90.0f, 91.0f, 92.0f);
    modalData.probePointCount = 99;
    modelNode.listCountB = 2;
    collisionNodes[0].flags = 0x04;
    collisionNodes[1].flags = 0x04;
    const int missing = Player::BuildCollisionPointsFromModel(&saveState, &modelNode);
    if (missing != 0 || (collisionNodes[0].flags & 0x04) != 0 ||
        (collisionNodes[1].flags & 0x04) != 0 || modalData.probePointCount != 99 ||
        !Vec3Equals(modalData.probePoints[0], zVec3_Make(90.0f, 91.0f, 92.0f))) {
        return 3;
    }

    return 0;
}

extern "C" int player_sample_ground_and_align_root_to_surface_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;

    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = reinterpret_cast<float *>(&identityMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial emptyWorld = {};
    zClass_WorldDataPartial emptyWorldData = {};

    saveState.playerState = &playerState;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.25f, 10.0f, 0.25f};
    playerState.restartYawRad = 0.25f;
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    rootNode.nodeType = 7;
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;
    g_Variant_CurrentTag.tags[2] = 3;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);
    if (rootNode.nodeType != 0xff || (rootNode.flags & 0x08) == 0 ||
        playerState.variantTag.count != 0 || playerState.variantTag.tags[0] != 0xff ||
        g_Variant_CurrentTag.count != 0 || g_Variant_CurrentTag.tags[0] != 0xff ||
        rootData.rotation.x != 0.0f || rootData.rotation.y != 0.0f ||
        rootData.rotation.z != 0.0f) {
        g_Player_RuntimeDiScene = oldRuntimeScene;
        g_Variant_CurrentTag = oldVariantCurrent;
        return 1;
    }

    zVec3 vertices[3] = {{0.0f, 12.0f, 0.0f}, {0.0f, 12.0f, 1.0f},
                         {1.0f, 12.0f, 0.0f}};
    std::int32_t indices[3] = {0, 1, 2};
    zModel_MaterialPartial material = {};
    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 3;
    diEntry.vertexIndices = indices;
    diEntry.material = &material;
    diEntry.variantTagInitialized = 0;
    diEntry.variantTag = 0xff;
    zDiPartial di = {};
    di.entryCount = 1;
    di.vertCount = 3;
    di.entries = &diEntry;
    di.verts = vertices;

    zClass_Object3DDataPartial groundData = {};
    groundData.flags = 8;
    zClass_NodePartial groundNode = {};
    groundNode.flags = 0x11c;
    groundNode.nodeType = 0x37;
    groundNode.classId = 5;
    groundNode.classData = &groundData;
    groundNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    groundNode.cachedBounds[0] = -10.0f;
    groundNode.cachedBounds[1] = 0.0f;
    groundNode.cachedBounds[2] = -10.0f;
    groundNode.cachedBounds[3] = 10.0f;
    groundNode.cachedBounds[4] = 20.0f;
    groundNode.cachedBounds[5] = 10.0f;
    zClass_NodePartial *groundParents[1] = {};
    zClass_NodePartial *worldChildren[1] = {&groundNode};
    zWorldAreaPartial area = {};
    area.childCount = 1;
    area.childList = worldChildren;
    zWorldAreaPartial *areaRows[1] = {&area};
    zClass_WorldDataPartial worldData = {};
    worldData.clampQueriesToBounds = 1;
    worldData.areaCellSizeX = 1.0f;
    worldData.areaCellSizeZ = 1.0f;
    worldData.areaInvSizeX = 1.0f;
    worldData.areaInvSizeZ = 1.0f;
    worldData.areaGridColCount = 1;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = areaRows;
    zClass_NodePartial world = {};
    world.classId = 2;
    world.classData = &worldData;
    world.listCountB = 1;
    world.listB = worldChildren;
    groundParents[0] = &world;
    groundNode.listCountA = 1;
    groundNode.listA = groundParents;

    g_Player_RuntimeDiScene = &world;
    rootNode.nodeType = 7;
    rootNode.flags = 0x08;
    rootData.rotation = {9.0f, 8.0f, 7.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 7;
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.steerBasisRaw = {3.0f, 4.0f, 5.0f};
    playerState.restartYawRad = 0.0f;
    playerState.vehiclePitchRad = 6.0f;
    playerState.vehicleRollRad = 7.0f;

    Player::SampleGroundAndAlignRootToSurface(&saveState, 1);

    int hitFailure = 0;
    if (rootNode.nodeType == 0xff) {
        hitFailure = 12;
    } else if (rootNode.nodeType == 0) {
        hitFailure = 13;
    } else if (rootNode.nodeType != 0x37) {
        hitFailure = 2;
    } else if ((rootNode.flags & 0x08) == 0) {
        hitFailure = 3;
    } else if (playerState.variantTag.count != 0) {
        hitFailure = 4;
    } else if (!FloatNear(playerState.steerBasisRef.x, 0.0f)) {
        hitFailure = 5;
    } else if (!FloatNear(playerState.steerBasisRef.z, 0.0f)) {
        hitFailure = 6;
    } else if (!FloatNear(playerState.vehiclePitchRad, 0.0f)) {
        hitFailure = 7;
    } else if (!FloatNear(playerState.vehicleRollRad, 0.0f)) {
        hitFailure = 8;
    } else if (!FloatNear(rootData.rotation.x, 0.0f)) {
        hitFailure = 9;
    } else if (!FloatNear(rootData.rotation.y, 0.0f)) {
        hitFailure = 10;
    } else if (!FloatNear(rootData.rotation.z, 0.0f)) {
        hitFailure = 11;
    }

    rootData.rotation = {3.0f, 4.0f, 5.0f};
    rootNode.nodeType = 7;
    Player::SampleGroundAndAlignRootToSurface(&saveState, 0);
    const bool noRotationOk = rootNode.nodeType == 0x37 &&
                              rootData.rotation.x == 3.0f &&
                              rootData.rotation.y == 4.0f &&
                              rootData.rotation.z == 5.0f;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    if (hitFailure != 0) {
        return hitFailure;
    }
    return noRotationOk ? 0 : 3;
}

extern "C" int player_init_spawn_state_from_primary_modal_data_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const float oldNominalGravity = g_Player_NominalGravity;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_Object3DDataPartial rootData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial worldNode = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootNode.flags = 0x08;
    worldNode.classData = &worldData;
    g_Player_RuntimeDiScene = &worldNode;
    g_Player_NominalGravity = 19.5f;

    playerState.spawnStateInitialized = 7;
    playerState.primaryGunGateUntilTime = 8.0f;
    playerState.gravityAccel = 1.0f;
    playerState.primaryFireSlotIndex = 3;
    playerState.altFireSlotIndex = 4;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 9;
    modalData.probePointCount = 3;
    modalData.probePoints[0] = {1.0f, 2.0f, 3.0f};
    modalData.probePoints[1] = {-4.0f, 5.0f, -6.0f};
    modalData.probePoints[2] = {7.0f, -8.0f, 9.0f};

    Player::InitSpawnStateFromPrimaryModalData(&saveState);

    const bool ok = playerState.spawnStateInitialized == 0 &&
                    playerState.primaryGunGateUntilTime == 0.0f &&
                    playerState.gravityAccel == 19.5f &&
                    playerState.primaryFireSlotIndex == 0 &&
                    playerState.altFireSlotIndex == 0 &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[0],
                               zVec3_Make(11.0f, 22.0f, 33.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[1],
                               zVec3_Make(6.0f, 25.0f, 24.0f)) &&
                    Vec3Equals(playerState.rootProbeWorldByIndex[2],
                               zVec3_Make(17.0f, 12.0f, 39.0f)) &&
                    rootNode.nodeType == 0xff && (rootNode.flags & 0x08) != 0 &&
                    playerState.variantTag.count == 0 &&
                    playerState.variantTag.tags[0] == 0xff;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_Player_NominalGravity = oldNominalGravity;
    return ok ? 0 : 1;
}

extern "C" int player_create_from_names_at_pose_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    zClass_TypeListLink *const oldType6Head = zClass_TypeList::Head(6);
    zClass_TypeListLink *const oldType6Tail = zClass_TypeList::Tail(6);
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const int oldMissionStat1 = g_HudSensorTracker.missionStat1;
    const float oldNominalGravity = g_Player_NominalGravity;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    zEffectAnimEntry *const oldEffectEntries = g_zEffectAnim_EntryList;
    const short oldEffectCount = g_zEffectAnim_EntryCount;
    zZbdManager *const oldZbdManager = g_zUtil_ZbdManager;
    AINet *const oldAiHead = g_AINetListHead;
    AINet *const oldAiTail = g_AINetListTail;
    int *const oldMatrixIdentityFlagSlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;
    zVec3 *const oldSharedScratchA = g_zModel_SharedVec3ScratchA;
    zVec3 *const oldSharedScratchB = g_zModel_SharedVec3ScratchB;

    int networkEnabled = 1;
    int gameControlOptions = 0;
    static std::int32_t matrixFlags[2];
    static float *matrixSlots[2];
    static zMat4x3 identityMatrix;
    identityMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                      0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixFlags[0] = 1;
    matrixSlots[0] = (float *)&identityMatrix;
    zClass_NodePartial world = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodeFreeListSlot rootSlot = {};
    zClass_NodePartial &rootNode = rootSlot.node;
    zClass_Object3DDataPartial rootData = {};
    zClass_TypeListLink rootLink = {&rootNode, 0, 0, 0};
    PlayerMasterCommonData commonData = {};
    PlayerMasterModalData modalData = {};

    world.classId = 2;
    world.classData = &worldData;
    std::strcpy(rootNode.name, "net_tank");
    rootNode.classId = 5;
    rootNode.classData = &rootData;
    rootData.localMatrix[0] = 1.0f;
    rootData.localMatrix[4] = 1.0f;
    rootData.localMatrix[8] = 1.0f;

    std::strcpy(commonData.vehicleName, "tank_common");
    std::strcpy(commonData.modalNames[0], "track");
    std::strcpy(commonData.startAnimsName, "startup");
    commonData.modalCount = 1;
    commonData.maxHealth = 150.0f;
    commonData.cambackDist0 = 12.0f;
    commonData.cambackSide0 = 1.0f;
    commonData.cambackBase0 = 2.0f;
    std::strcpy(modalData.modalName, "tank_common");
    std::strcpy(modalData.modeName, "track");
    modalData.probePointCount = 1;
    modalData.platformPointCount = 1;
    modalData.probePoints[0] = zVec3_Make(0.5f, 1.0f, 1.5f);

    g_PlayerSaveStateListHead = 0;
    g_PlayerSaveStateListTail = 0;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_PlayerMasterCommonDataHead = &commonData;
    g_PlayerMasterCommonDataTail = &commonData;
    g_PlayerMasterCommonDataListAux = 1;
    g_PlayerMasterCommonDataCount = 1;
    g_PlayerMasterModalDataHead = &modalData;
    g_PlayerMasterModalDataTail = &modalData;
    g_PlayerMasterModalDataListAux = 1;
    g_PlayerMasterModalDataCount = 1;
    zClass_TypeList::Head(6) = &rootLink;
    zClass_TypeList::Tail(6) = &rootLink;
    g_Player_RuntimeDiScene = &world;
    g_GameStateOrMapTable = 0;
    g_Player_NextOrdinal = 1;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = 19.5f;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zEffectAnim_EntryList = 0;
    g_zEffectAnim_EntryCount = 0;
    g_zUtil_ZbdManager = 0;
    g_AINetListHead = 0;
    g_AINetListTail = 0;
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;

    const zVec3 spawnPos = {11.0f, 22.0f, 33.0f};
    const int createResult =
        Player::CreateFromNamesAtPose(&spawnPos, 77, 90.0f, "tank_common", "net_tank");

    zUtil_SaveGameState *const createdSave = g_PlayerSaveStateListHead;
    zUtil_PlayerStateStorage *const playerState =
        createdSave != 0 ? createdSave->playerState : 0;
    PlayerModalState *const modalState =
        createdSave != 0 ? createdSave->primaryModalState : 0;
    zUtil_SaveGameState *wrapperSave = 0;
    PlayerModalState *wrapperModalState = 0;

    int result = 0;
    if (createResult != 1 ||
        createdSave == 0 ||
        playerState == 0 ||
        g_PlayerSaveStateListTail != createdSave ||
        g_PlayerSaveStateCount != 1) {
        result = 1;
    } else if (playerState->rootNode != &rootNode ||
               playerState->aiNetId != 77 ||
               !FloatNear(rootData.localMatrix[9], 11.0f) ||
               !FloatNear(rootData.localMatrix[10], 22.0f) ||
               !FloatNear(rootData.localMatrix[11], 33.0f) ||
               !FloatNear(playerState->restartYawRad, 1.57079637f)) {
        result = 2;
    } else if (world.listCountB != 1 ||
               world.listB == 0 ||
               world.listB[0] != &rootNode ||
               rootNode.listCountA != 1 ||
               rootNode.listA == 0 ||
               rootNode.listA[0] != &world) {
        result = 3;
    } else if (playerState->masterCommonData != &commonData ||
               playerState->statusMeterValue != 150.0f ||
               modalState == 0 ||
               modalState->masterModalData != &modalData ||
               createdSave->primaryModalState != modalState) {
        result = 4;
    } else if (playerState->gravityAccel != 19.5f ||
               !Vec3Equals(
                   playerState->rootProbeWorldByIndex[0],
                   zVec3_Make(11.5f, 23.0f, 34.5f)
               ) ||
               g_GameStateOrMapTable != (zInput_GameStateOrMapTablePartial *)createdSave ||
               g_Player_NextOrdinal != 2 ||
               g_HudSensorTracker.missionStat1 != oldMissionStat1) {
        result = 5;
    } else {
        const zVec3 wrapperSpawnPos = {44.0f, 55.0f, 66.0f};
        wrapperSave = Player::CreateFromNamesAtPoseGetState(
            &wrapperSpawnPos,
            "tank_common",
            180.0f,
            "net_tank"
        );
        zUtil_PlayerStateStorage *const wrapperState =
            wrapperSave != 0 ? wrapperSave->playerState : 0;
        wrapperModalState = wrapperSave != 0 ? wrapperSave->primaryModalState : 0;
        if (wrapperSave == 0 ||
            wrapperSave != g_PlayerSaveStateListTail ||
            createdSave->next != wrapperSave ||
            g_PlayerSaveStateCount != 2 ||
            wrapperState == 0 ||
            wrapperState->aiNetId != 0 ||
            !FloatNear(wrapperState->restartYawRad, 3.14159274f) ||
            !FloatNear(rootData.localMatrix[9], 44.0f) ||
            !FloatNear(rootData.localMatrix[10], 55.0f) ||
            !FloatNear(rootData.localMatrix[11], 66.0f) ||
            g_Player_NextOrdinal != 3) {
            result = 6;
        }
    }

    if (wrapperSave != 0) {
        if (wrapperModalState != 0) {
            std::free(wrapperModalState);
        }
        std::free(wrapperSave->playerState);
        ::operator delete(wrapperSave);
    }
    if (createdSave != 0) {
        if (modalState != 0) {
            std::free(modalState);
        }
        std::free(createdSave->playerState);
        ::operator delete(createdSave);
    }
    std::free(world.listB);
    std::free(rootNode.listA);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    zClass_TypeList::Head(6) = oldType6Head;
    zClass_TypeList::Tail(6) = oldType6Tail;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    g_GameStateOrMapTable = oldGameState;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_HudSensorTracker.missionStat1 = oldMissionStat1;
    g_Player_NominalGravity = oldNominalGravity;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zEffectAnim_EntryList = oldEffectEntries;
    g_zEffectAnim_EntryCount = oldEffectCount;
    g_zUtil_ZbdManager = oldZbdManager;
    g_AINetListHead = oldAiHead;
    g_AINetListTail = oldAiTail;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentityFlagSlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;
    g_zModel_SharedVec3ScratchA = oldSharedScratchA;
    g_zModel_SharedVec3ScratchB = oldSharedScratchB;
    return result;
}

extern "C" int player_init_mission_runtime_missing_aiv_smoke(void) {
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldSaveTail = g_PlayerSaveStateListTail;
    const int oldSaveAux = g_PlayerSaveStateListAux;
    const int oldSaveCount = g_PlayerSaveStateCount;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    PlayerMasterCommonData *const oldCommonHead = g_PlayerMasterCommonDataHead;
    PlayerMasterCommonData *const oldCommonTail = g_PlayerMasterCommonDataTail;
    const int oldCommonAux = g_PlayerMasterCommonDataListAux;
    const int oldCommonCount = g_PlayerMasterCommonDataCount;
    PlayerMasterModalData *const oldModalHead = g_PlayerMasterModalDataHead;
    PlayerMasterModalData *const oldModalTail = g_PlayerMasterModalDataTail;
    const int oldModalAux = g_PlayerMasterModalDataListAux;
    const int oldModalCount = g_PlayerMasterModalDataCount;
    const int oldMissionInitFirstRun = g_Player_MissionInitFirstRunFlag;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const HudUiPanel oldTopPanel1 = g_Player_TopMsgPanel1;
    const HudUiPanel oldTopPanel2 = g_Player_TopMsgPanel2;
    const Player_UnderwaterFxPass3Ui oldUnderwaterFxPass3Ui =
        g_Player_UnderwaterFxPass3Ui;
    const Player_ProjectileCameraFxPass3Ui oldState7FxPass3Ui =
        g_Player_State7FxPass3Ui;
    HudUiContainer *const fxContainer =
        reinterpret_cast<HudUiContainer *>(&g_zVideo_FxPass3ConfigLocal);
    const HudUiContainer oldFxContainer = *fxContainer;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    zClass_NodePartial *const oldHorizonNode = g_Player_HorizonNode;
    const int oldHorizonEnabled = g_Player_HorizonNodeFollowCameraEnabled;
    const int oldRuntimeInputFlags = g_Player_RuntimeInputFlags;
    const int oldLocalControlEnabled = g_Player_LocalControlEnabled;
    const int oldNextOrdinal = g_Player_NextOrdinal;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldCameraZone = g_Player_CameraZone;
    const float oldCameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float oldNominalGravity = g_Player_NominalGravity;
    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    zClass_NodeFreeListSlot *const oldNodeArray = g_zClass_NodeArray;
    const int oldFreeHead = g_zClass_NodeFreeHeadIndex;
    const int oldActiveNodeCount = g_zClass_ActiveNodeCount;
    zClass_TypeListLink *const oldFreeLinkHead = g_zClass_TypeList_FreeLinkHead;
    zClass_TypeListLink *const oldPendingFreeHead = g_zClass_NodeList_PendingFreeHead;
    const int oldDeferredProcessing = g_zClass_DeferredProcessingEnabled;
    const int oldLiveLinkCount = g_zClass_TypeList_LiveLinkCount;
    const int oldPeakLiveLinkCount = g_zClass_TypeList_PeakLiveLinkCount;
    zClass_NodePartial *const oldHudWorldNode = g_HudSensorTracker.worldNode;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    int *const oldGameControlOptions = ZOPT_GAME_CONTROL_OPTIONS;
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zClass_TypeListLink *oldTypeHeads[16] = {};
    zClass_TypeListLink *oldTypeTails[16] = {};
    int oldTypeDirty[16] = {};
    for (int i = 0; i < 16; ++i) {
        oldTypeHeads[i] = zClass_TypeList::Head(i);
        oldTypeTails[i] = zClass_TypeList::Tail(i);
        oldTypeDirty[i] = zClass_TypeList::PendingRemovalDirty(i);
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    char tempPath[MAX_PATH] = {};
    char tempFile[MAX_PATH] = {};
    if (
        GetTempPathA(sizeof(tempPath), tempPath) == 0 ||
        GetTempFileNameA(tempPath, "pim", 0, tempFile) == 0
    ) {
        return 1;
    }

    zReader::Node playerRoot = {};
    zReader::Node playerItems[3] = {};
    zReader::Node cameraZoneItems[2] = {};
    MakeAinetReaderArrayNode(playerRoot, playerItems, 3);
    MakeAinetReaderStringNode(playerItems[1], "camera_zone");
    MakeAinetReaderArrayNode(playerItems[2], cameraZoneItems, 2);
    MakeAinetReaderFloatNode(cameraZoneItems[1], 0.75f);

    zReader::Node vehicleRoot = {};
    zReader::Node vehicleItems[3] = {};
    zReader::Node stealthItems[5] = {};
    zReader::Node commonItems[1] = {};
    zReader::Node modalItems[3] = {};
    zReader::Node modeItems[2] = {};
    MakeAinetReaderArrayNode(vehicleRoot, vehicleItems, 3);
    MakeAinetReaderStringNode(vehicleItems[1], "stealth");
    MakeAinetReaderArrayNode(vehicleItems[2], stealthItems, 5);
    MakeAinetReaderStringNode(stealthItems[1], "common_mode");
    MakeAinetReaderArrayNode(stealthItems[2], commonItems, 1);
    MakeAinetReaderStringNode(stealthItems[3], "basic");
    MakeAinetReaderArrayNode(stealthItems[4], modalItems, 3);
    MakeAinetReaderStringNode(modalItems[1], "mode");
    MakeAinetReaderArrayNode(modalItems[2], modeItems, 2);
    MakeAinetReaderStringNode(modeItems[1], "basic");

    const AinetZrdArchiveEntry entries[] = {
        {"player.zrd", &playerRoot},
        {"vehicle.zrd", &vehicleRoot},
    };
    zIndexArchive archive = {};
    zZarFileRecord records[2] = {};
    zArchiveListNode archiveNode = {};
    zArchiveList archiveList = {};
    if (
        !MountAinetZrdArchive(
            tempFile,
            entries,
            2,
            archive,
            records,
            archiveNode,
            archiveList
        )
    ) {
        return 2;
    }

    HudUiTopMessageStack topStack = {};
    topStack.Constructor();
    HudUiElement *const oldTopStackTail = topStack.childTail;
    g_HudUiTopMessageStack = &topStack;
    Player_TopMsgPanel1::Constructor();
    Player_TopMsgPanel2::Constructor();
    g_Player_UnderwaterFxPass3Ui.Constructor();
    g_Player_State7FxPass3Ui.Constructor();
    *fxContainer = {};

    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    worldNode.classId = 2;
    worldNode.classData = &worldData;
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.frustumWidth = 1.25f;
    cameraData.frustumHeight = 0.75f;

    zClass_NodeFreeListSlot slots[8] = {};
    for (int i = 0; i < 7; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[7].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    int networkEnabled = 1;
    int gameControlOptions = 0;
    int difficultyOption = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControlOptions;
    g_zOpt_GameDifficultyOption = &difficultyOption;
    g_PlayerSaveStateListHead = nullptr;
    g_PlayerSaveStateListTail = nullptr;
    g_PlayerSaveStateListAux = 0;
    g_PlayerSaveStateCount = 0;
    g_Player2SaveState = nullptr;
    g_LocalPlayerSaveState = nullptr;
    g_CurrentPlayerSaveState = nullptr;
    g_GameStateOrMapTable = nullptr;
    g_PlayerMasterCommonDataHead = nullptr;
    g_PlayerMasterCommonDataTail = nullptr;
    g_PlayerMasterCommonDataListAux = 0;
    g_PlayerMasterCommonDataCount = 0;
    g_PlayerMasterModalDataHead = nullptr;
    g_PlayerMasterModalDataTail = nullptr;
    g_PlayerMasterModalDataListAux = 0;
    g_PlayerMasterModalDataCount = 0;
    g_Player_MissionInitFirstRunFlag = 1;
    g_Player_HorizonNode = nullptr;
    g_Player_HorizonNodeFollowCameraEnabled = 0;
    g_Player_RuntimeInputFlags = 0;
    g_Player_LocalControlEnabled = 0;
    g_Player_NextOrdinal = 1;
    g_Time_AccumulatedTimeSec = 9.5f;
    g_Player_TotalTimeSecScaled = 9.5f;
    g_Player_CameraZone = 0.5f;
    g_Player_CameraZoneInvRange = 2.0f;
    g_Player_NominalGravity = 0.0f;
    g_PlayerStatusMeterRatio = 0.0f;
    g_HudSensorTracker.worldNode = &worldNode;
    Player::InitMissionRuntimeFromWorldAndCamera(&worldNode, &cameraNode);

    zUtil_SaveGameState *const stealthSave = g_Player2SaveState;
    zUtil_PlayerStateStorage *const stealthState =
        stealthSave != nullptr ? stealthSave->playerState : nullptr;
    PlayerMasterCommonData *const commonData = g_PlayerMasterCommonDataHead;
    PlayerMasterModalData *const modalData = g_PlayerMasterModalDataHead;

    int result = 0;
    if (
        g_Player_MissionInitFirstRunFlag != 0 ||
        oldTopStackTail == nullptr ||
        oldTopStackTail->next != reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1) ||
        reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1)->next !=
            reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2) ||
        topStack.childTail != reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2)
    ) {
        result = 3;
    } else if (
        g_Player_RuntimeDiScene != &worldNode ||
        g_MainCamera != &cameraNode ||
        cameraData.posOffset.x != 0.0f ||
        cameraData.posOffset.y != 0.0f ||
        cameraData.posOffset.z != 0.0f ||
        cameraData.targetOrEuler.x != 0.0f ||
        cameraData.targetOrEuler.y != 0.0f ||
        cameraData.targetOrEuler.z != 0.0f
    ) {
        result = 4;
    } else if (
        (reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel1)->flags & 0x10) == 0 ||
        (reinterpret_cast<HudUiElement *>(&g_Player_TopMsgPanel2)->flags & 0x10) == 0 ||
        (reinterpret_cast<HudUiElement *>(&g_Player_UnderwaterFxPass3Ui)->flags & 0x10) == 0 ||
        (reinterpret_cast<HudUiElement *>(&g_Player_State7FxPass3Ui)->flags & 0x10) == 0
    ) {
        result = 5;
    } else if (
        g_Player_LocalControlEnabled != 1 ||
        g_Player_RuntimeInputFlags != 3 ||
        !FloatNear(g_Player_TotalTimeSecScaled, 9.5f) ||
        !FloatNear(g_Player_CameraZone, 0.75f) ||
        !FloatNear(g_Player_CameraZoneInvRange, 4.0f) ||
        !FloatNear(g_Player_NominalGravity, 28.0f) ||
        !FloatNear(g_PlayerStatusMeterRatio, 1.0f)
    ) {
        result = 6;
    } else if (
        commonData == nullptr ||
        modalData == nullptr ||
        g_PlayerMasterCommonDataCount != 1 ||
        g_PlayerMasterModalDataCount != 1 ||
        std::strcmp(commonData->vehicleName, "stealth") != 0 ||
        commonData->modalCount != 1 ||
        std::strcmp(modalData->modalName, "stealth") != 0 ||
        std::strcmp(modalData->modeName, "basic") != 0
    ) {
        result = 7;
    } else if (
        stealthSave == nullptr ||
        stealthSave != g_PlayerSaveStateListHead ||
        stealthState == nullptr ||
        stealthState->rootNode == nullptr ||
        std::strcmp(stealthState->rootNode->name, "Stealth") != 0 ||
        stealthState->lifecycleState != 4 ||
        stealthState->cameraState != 3 ||
        !FloatNear(stealthState->worldPos.x, 500.0f) ||
        !FloatNear(stealthState->worldPos.y, 50.0f) ||
        !FloatNear(stealthState->worldPos.z, 500.0f)
    ) {
        result = 8;
    } else if (
        g_LocalPlayerSaveState != nullptr ||
        g_CurrentPlayerSaveState != nullptr ||
        g_PlayerSaveStateCount != 1 ||
        g_GameStateOrMapTable !=
            reinterpret_cast<zInput_GameStateOrMapTablePartial *>(stealthSave)
    ) {
        result = 9;
    }

    Player::ShutdownMissionRuntime();
    for (int i = 0; i < 8; ++i) {
        std::free(slots[i].node.classData);
        slots[i].node.classData = nullptr;
    }
    if (archive.hFile != INVALID_HANDLE_VALUE && archive.hFile != nullptr) {
        CloseHandle(archive.hFile);
    }
    DeleteFileA(tempFile);

    g_PlayerSaveStateListHead = oldSaveHead;
    g_PlayerSaveStateListTail = oldSaveTail;
    g_PlayerSaveStateListAux = oldSaveAux;
    g_PlayerSaveStateCount = oldSaveCount;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_GameStateOrMapTable = oldGameState;
    g_PlayerMasterCommonDataHead = oldCommonHead;
    g_PlayerMasterCommonDataTail = oldCommonTail;
    g_PlayerMasterCommonDataListAux = oldCommonAux;
    g_PlayerMasterCommonDataCount = oldCommonCount;
    g_PlayerMasterModalDataHead = oldModalHead;
    g_PlayerMasterModalDataTail = oldModalTail;
    g_PlayerMasterModalDataListAux = oldModalAux;
    g_PlayerMasterModalDataCount = oldModalCount;
    g_Player_MissionInitFirstRunFlag = oldMissionInitFirstRun;
    g_HudUiTopMessageStack = oldTopStack;
    g_Player_TopMsgPanel1 = oldTopPanel1;
    g_Player_TopMsgPanel2 = oldTopPanel2;
    g_Player_UnderwaterFxPass3Ui = oldUnderwaterFxPass3Ui;
    g_Player_State7FxPass3Ui = oldState7FxPass3Ui;
    *fxContainer = oldFxContainer;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_MainCamera = oldMainCamera;
    g_Player_HorizonNode = oldHorizonNode;
    g_Player_HorizonNodeFollowCameraEnabled = oldHorizonEnabled;
    g_Player_RuntimeInputFlags = oldRuntimeInputFlags;
    g_Player_LocalControlEnabled = oldLocalControlEnabled;
    g_Player_NextOrdinal = oldNextOrdinal;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Player_CameraZone = oldCameraZone;
    g_Player_CameraZoneInvRange = oldCameraZoneInvRange;
    g_Player_NominalGravity = oldNominalGravity;
    g_PlayerStatusMeterRatio = oldStatusMeterRatio;
    g_zClass_NodeArray = oldNodeArray;
    g_zClass_NodeFreeHeadIndex = oldFreeHead;
    g_zClass_ActiveNodeCount = oldActiveNodeCount;
    g_zClass_TypeList_FreeLinkHead = oldFreeLinkHead;
    g_zClass_NodeList_PendingFreeHead = oldPendingFreeHead;
    g_zClass_DeferredProcessingEnabled = oldDeferredProcessing;
    g_zClass_TypeList_LiveLinkCount = oldLiveLinkCount;
    g_zClass_TypeList_PeakLiveLinkCount = oldPeakLiveLinkCount;
    g_HudSensorTracker.worldNode = oldHudWorldNode;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_GAME_CONTROL_OPTIONS = oldGameControlOptions;
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    g_zArchive_MountedList = oldMountedList;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = oldTypeHeads[i];
        zClass_TypeList::Tail(i) = oldTypeTails[i];
        zClass_TypeList::PendingRemovalDirty(i) = oldTypeDirty[i];
    }

    return result;
}

extern "C" int player_mgr_tick_all_players_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldLocalSaveState = g_LocalPlayerSaveState;
    zUtil_SaveGameState *const oldPlayer2SaveState = g_Player2SaveState;
    zUtil_SaveGameState *const oldCurrentSaveState = g_CurrentPlayerSaveState;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    int *const oldAudioApi = ZOPT_AUDIO_API;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    const float oldFrameDelta = g_FrameDeltaTimeSec;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;
    const float oldPlayerDelta = g_Player_DeltaTime;
    const float oldInvDelta = g_Player_InvDeltaTime;
    const float oldScaled001 = g_Player_DeltaTimeScaled001;
    const float oldTotalScaled = g_Player_TotalTimeSecScaled;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    int audioApi = 0;
    int networkEnabled = 0;
    ZOPT_AUDIO_API = &audioApi;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    zUtil_SaveGameState localSaveState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    localSaveState.playerState = &localPlayerState;
    localPlayerState.lifecycleState = 4;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;
    g_CurrentPlayerSaveState = 0;
    g_PlayerSaveStateListHead = 0;
    g_LocalPlayerSaveState = 0;
    g_Player2SaveState = 0;

    g_FrameDeltaTimeSec = 0.001f;
    g_Time_AccumulatedTimeSec = 77.0f;
    Player::TickAllPlayers();
    const bool emptyListOk =
        FloatNear(g_Player_DeltaTime, 0.00499999989f) &&
        FloatNear(g_Player_InvDeltaTime, 200.0f) &&
        FloatNear(g_Player_DeltaTimeScaled001, 0.000049999997f) &&
        FloatNear(g_Player_TotalTimeSecScaled, 77.0f);

    zUtil_SaveGameState aiSaveState = {};
    zUtil_PlayerStateStorage aiPlayerState = {};
    aiSaveState.playerState = &aiPlayerState;
    g_PlayerSaveStateListHead = &aiSaveState;
    g_FrameDeltaTimeSec = 0.02f;
    g_Time_AccumulatedTimeSec = 88.0f;
    aiPlayerState.lifecycleState = 2;
    aiPlayerState.generalFlags = 0;
    aiPlayerState.aiActive = 9;
    aiPlayerState.targetDistanceSq = 123.0f;
    aiPlayerState.variantTag.count = 1;
    aiPlayerState.variantTag.tags[0] = 7;
    aiPlayerState.variantTag.tags[1] = 0xff;
    aiPlayerState.variantTag.tags[2] = 0xff;
    g_VariantTag_Current.count = 1;
    g_VariantTag_Current.tags[0] = 9;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;
    Player::TickAllPlayers();
    const bool inactiveAiOk =
        (aiPlayerState.generalFlags & 2) != 0 &&
        aiPlayerState.aiActive == 0 &&
        FloatNear(aiPlayerState.targetDistanceSq, 123.0f) &&
        FloatNear(g_Player_DeltaTime, 0.02f) &&
        FloatNear(g_Player_InvDeltaTime, 50.0f) &&
        FloatNear(g_Player_DeltaTimeScaled001, 0.00019999999f) &&
        FloatNear(g_Player_TotalTimeSecScaled, 88.0f);

    g_VariantTag_Current = oldVariantTagCurrent;
    g_Player_TotalTimeSecScaled = oldTotalScaled;
    g_Player_DeltaTimeScaled001 = oldScaled001;
    g_Player_InvDeltaTime = oldInvDelta;
    g_Player_DeltaTime = oldPlayerDelta;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_FrameDeltaTimeSec = oldFrameDelta;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    ZOPT_AUDIO_API = oldAudioApi;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_CurrentPlayerSaveState = oldCurrentSaveState;
    g_Player2SaveState = oldPlayer2SaveState;
    g_LocalPlayerSaveState = oldLocalSaveState;
    g_PlayerSaveStateListHead = oldHead;

    if (!emptyListOk) {
        return 1;
    }
    return inactiveAiOk ? 0 : 2;
}

extern "C" int player_node_flag_restore_init_globals_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    PlayerNodeFlagRestoreEntry sentinels[2] = {};
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0x5a;
    g_PlayerNodeFlagRestoreEntriesBegin = sentinels;
    g_PlayerNodeFlagRestoreEntriesEnd = sentinels + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = sentinels + 2;

    PlayerNodeFlagRestore::InitGlobals();

    const bool clearedOk =
        g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy == 0 &&
        g_PlayerNodeFlagRestoreEntriesBegin == 0 &&
        g_PlayerNodeFlagRestoreEntriesEnd == 0 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == 0;

    atexit(ClearPlayerNodeFlagRestoreGlobalsAtExit);
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    return clearedOk ? 0 : 1;
}

extern "C" int player_node_flag_restore_init_instance_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    PlayerNodeFlagRestoreEntry sentinels[2] = {};
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0x5a;
    g_PlayerNodeFlagRestoreEntriesBegin = sentinels;
    g_PlayerNodeFlagRestoreEntriesEnd = sentinels + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = sentinels + 2;

    PlayerNodeFlagRestore::InitInstance();

    const bool clearedOk =
        g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy == 0 &&
        g_PlayerNodeFlagRestoreEntriesBegin == 0 &&
        g_PlayerNodeFlagRestoreEntriesEnd == 0 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == 0;

    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    return clearedOk ? 0 : 1;
}

extern "C" int player_node_flag_restore_register_at_exit_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    PlayerNodeFlagRestoreEntry sentinels[2] = {};
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0x5a;
    g_PlayerNodeFlagRestoreEntriesBegin = sentinels;
    g_PlayerNodeFlagRestoreEntriesEnd = sentinels + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = sentinels + 2;

    PlayerNodeFlagRestore::RegisterAtExit();

    const bool unchangedOk =
        g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy == 0x5a &&
        g_PlayerNodeFlagRestoreEntriesBegin == sentinels &&
        g_PlayerNodeFlagRestoreEntriesEnd == sentinels + 1 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == sentinels + 2;

    atexit(ClearPlayerNodeFlagRestoreGlobalsAtExit);
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    return unchangedOk ? 0 : 1;
}

extern "C" int player_node_flag_restore_shutdown_instance_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    PlayerNodeFlagRestoreEntry *const testBegin = (PlayerNodeFlagRestoreEntry *)(::operator new(
        sizeof(PlayerNodeFlagRestoreEntry) * 2
    ));
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0x5a;
    g_PlayerNodeFlagRestoreEntriesBegin = testBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = testBegin + 1;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = testBegin + 2;

    PlayerNodeFlagRestore::ShutdownInstance();

    const bool clearedOk =
        g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy == 0x5a &&
        g_PlayerNodeFlagRestoreEntriesBegin == 0 &&
        g_PlayerNodeFlagRestoreEntriesEnd == 0 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == 0;

    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    return clearedOk ? 0 : 1;
}

extern "C" int player_restore_recorded_node_flags_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    zClass_NodePartial untouchedNode = {};
    zClass_NodePartial allFlagsNode = {};
    zClass_NodePartial raycastOnlyNode = {};
    raycastOnlyNode.flags = 0x20;

    PlayerNodeFlagRestoreEntry entries[3] = {};
    entries[0].node = &untouchedNode;
    entries[0].wasCellPickable = 0;
    entries[0].wasRaycastable = 0;
    entries[0].wasPickable = 0;
    entries[1].node = &allFlagsNode;
    entries[1].wasCellPickable = 1;
    entries[1].wasRaycastable = 1;
    entries[1].wasPickable = 1;
    entries[2].node = &raycastOnlyNode;
    entries[2].wasCellPickable = 0;
    entries[2].wasRaycastable = 1;
    entries[2].wasPickable = 0;

    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0;
    g_PlayerNodeFlagRestoreEntriesBegin = entries;
    g_PlayerNodeFlagRestoreEntriesEnd = entries + 3;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = entries + 3;

    Player::RestoreRecordedNodeFlags();

    const bool flagsOk =
        untouchedNode.flags == 0 &&
        (allFlagsNode.flags & 0x38) == 0x38 &&
        (raycastOnlyNode.flags & 0x10) != 0 &&
        (raycastOnlyNode.flags & 0x08) == 0 &&
        (raycastOnlyNode.flags & 0x20) != 0;

    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    return flagsOk ? 0 : 1;
}

extern "C" int player_record_node_flags_for_restore_smoke(void) {
    const unsigned char oldProxy = g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy;
    PlayerNodeFlagRestoreEntry *const oldBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    PlayerNodeFlagRestoreEntry *const oldEnd = g_PlayerNodeFlagRestoreEntriesEnd;
    PlayerNodeFlagRestoreEntry *const oldCapacityEnd = g_PlayerNodeFlagRestoreEntriesCapacityEnd;

    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = 0;
    g_PlayerNodeFlagRestoreEntriesBegin = nullptr;
    g_PlayerNodeFlagRestoreEntriesEnd = nullptr;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = nullptr;

    zClass_NodePartial firstNode = {};
    zClass_NodePartial secondNode = {};
    zClass_NodePartial thirdNode = {};
    firstNode.flags = 0x08 | 0x20;
    secondNode.flags = 0x10;
    thirdNode.flags = 0x08 | 0x10 | 0x20;

    Player::RecordNodeFlagsForRestore(&firstNode);
    const bool firstGrowthOk =
        g_PlayerNodeFlagRestoreEntriesBegin != nullptr &&
        g_PlayerNodeFlagRestoreEntriesEnd == g_PlayerNodeFlagRestoreEntriesBegin + 1 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == g_PlayerNodeFlagRestoreEntriesBegin + 1;

    Player::RecordNodeFlagsForRestore(&secondNode);
    const bool secondGrowthOk =
        g_PlayerNodeFlagRestoreEntriesEnd == g_PlayerNodeFlagRestoreEntriesBegin + 2 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == g_PlayerNodeFlagRestoreEntriesBegin + 2;

    Player::RecordNodeFlagsForRestore(&thirdNode);
    PlayerNodeFlagRestoreEntry *const testBegin = g_PlayerNodeFlagRestoreEntriesBegin;
    const bool thirdGrowthOk =
        g_PlayerNodeFlagRestoreEntriesEnd == testBegin + 3 &&
        g_PlayerNodeFlagRestoreEntriesCapacityEnd == testBegin + 4;

    const bool entriesOk =
        testBegin != nullptr &&
        testBegin[0].node == &firstNode &&
        testBegin[0].wasCellPickable == 1 &&
        testBegin[0].wasRaycastable == 0 &&
        testBegin[0].wasPickable == 1 &&
        testBegin[1].node == &secondNode &&
        testBegin[1].wasCellPickable == 0 &&
        testBegin[1].wasRaycastable == 1 &&
        testBegin[1].wasPickable == 0 &&
        testBegin[2].node == &thirdNode &&
        testBegin[2].wasCellPickable == 1 &&
        testBegin[2].wasRaycastable == 1 &&
        testBegin[2].wasPickable == 1;

    ::operator delete(testBegin);
    g_PlayerNodeFlagRestoreEntriesAllocatorOrProxy = oldProxy;
    g_PlayerNodeFlagRestoreEntriesBegin = oldBegin;
    g_PlayerNodeFlagRestoreEntriesEnd = oldEnd;
    g_PlayerNodeFlagRestoreEntriesCapacityEnd = oldCapacityEnd;

    if (!firstGrowthOk) {
        return 1;
    }
    if (!secondGrowthOk) {
        return 2;
    }
    if (!thirdGrowthOk) {
        return 3;
    }
    return entriesOk ? 0 : 4;
}

extern "C" int player_ai_restore_saved_top_level_state_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.aiTopLevelState = 1;
    playerState.aiSavedTopLevelState = 7;

    AINet::AiRestoreSavedTopLevelState(&saveState);

    return playerState.aiTopLevelState == 7 &&
                   playerState.aiSavedTopLevelState == 7
               ? 0
               : 1;
}

extern "C" int player_ai_finalize_mode2_state1_for_all_players_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    const int oldFinalized = g_Player_AiMode2State1Finalized;

    zUtil_SaveGameState matchingSaveState = {};
    zUtil_SaveGameState nonmatchingAiSaveState = {};
    zUtil_SaveGameState inactiveSaveState = {};
    zUtil_PlayerStateStorage matchingState = {};
    zUtil_PlayerStateStorage nonmatchingAiState = {};
    zUtil_PlayerStateStorage inactiveState = {};

    matchingSaveState.next = &nonmatchingAiSaveState;
    nonmatchingAiSaveState.next = &inactiveSaveState;
    matchingSaveState.playerState = &matchingState;
    nonmatchingAiSaveState.playerState = &nonmatchingAiState;
    inactiveSaveState.playerState = &inactiveState;

    matchingState.lifecycleState = 2;
    matchingState.aiTopLevelState = 1;
    matchingState.aiSavedTopLevelState = 4;
    nonmatchingAiState.lifecycleState = 2;
    nonmatchingAiState.aiTopLevelState = 3;
    nonmatchingAiState.aiSavedTopLevelState = 8;
    inactiveState.lifecycleState = 4;
    inactiveState.aiTopLevelState = 1;
    inactiveState.aiSavedTopLevelState = 9;

    g_PlayerSaveStateListHead = &matchingSaveState;
    g_Player_AiMode2State1Finalized = 0;
    AINet::AiFinalizeMode2State1ForAllPlayers();

    const bool populatedOk =
        matchingState.aiTopLevelState == 4 &&
        nonmatchingAiState.aiTopLevelState == 3 &&
        inactiveState.aiTopLevelState == 1 &&
        g_Player_AiMode2State1Finalized == 1;

    g_PlayerSaveStateListHead = 0;
    g_Player_AiMode2State1Finalized = 0;
    AINet::AiFinalizeMode2State1ForAllPlayers();
    const bool emptyOk = g_Player_AiMode2State1Finalized == 1;

    g_PlayerSaveStateListHead = oldHead;
    g_Player_AiMode2State1Finalized = oldFinalized;

    if (!populatedOk) {
        return 1;
    }
    return emptyOk ? 0 : 2;
}

extern "C" int player_ai_steer_toward_path_node_forward_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    currentNode.neighborNodes[0] = &forwardNode;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    g_Player_TotalTimeSecScaled = 40.0f;

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {3.0f, 10.0f, 0.0f};
    AINet::AiSteerTowardPathNodeForward(&saveState);
    const bool advanceOk =
        playerState.aiCurrentPathNode == &forwardNode &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        FloatNear(playerState.unknown_0fa4, 44.0f);

    playerState.aiCurrentPathNode = &currentNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {10.0f, 0.0f, 10.0f};
    AINet::AiSteerTowardPathNodeForward(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool forwardOk =
        FloatNear(playerState.throttleInput, 1.0f - diagonal) &&
        FloatNear(playerState.throttleInputCopy, 1.0f - diagonal) &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    playerState.aiCurrentPathNode = &currentNode;
    forwardNode.position = {-10.0f, 0.0f, 10.0f};
    AINet::AiSteerTowardPathNodeForward(&saveState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!advanceOk) {
        return 1;
    }
    if (!forwardOk) {
        return 2;
    }
    return behindOk ? 0 : 3;
}

extern "C" int player_ai_steer_toward_path_node_reverse_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    currentNode.neighborNodes[0] = &forwardNode;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    g_Player_TotalTimeSecScaled = 40.0f;

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {3.0f, 10.0f, 0.0f};
    AINet::AiSteerTowardPathNodeReverse(&saveState);
    const bool advanceOk =
        playerState.aiCurrentPathNode == &forwardNode &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        FloatNear(playerState.unknown_0fa4, 54.0f);

    playerState.aiCurrentPathNode = &currentNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    forwardNode.position = {-10.0f, 0.0f, 10.0f};
    AINet::AiSteerTowardPathNodeReverse(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool reverseForwardOk =
        FloatNear(playerState.throttleInput, -(1.0f - diagonal)) &&
        FloatNear(playerState.throttleInputCopy, -(1.0f - diagonal)) &&
        FloatNear(playerState.steeringInput, diagonal) &&
        FloatNear(playerState.steeringInputCopy, diagonal);

    playerState.aiCurrentPathNode = &currentNode;
    forwardNode.position = {10.0f, 0.0f, 10.0f};
    AINet::AiSteerTowardPathNodeReverse(&saveState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!advanceOk) {
        return 1;
    }
    if (!reverseForwardOk) {
        return 2;
    }
    return behindOk ? 0 : 3;
}

extern "C" int zvehicle_select_zrd_by_difficulty_smoke(void) {
    int *const oldDifficultyOption = g_zOpt_GameDifficultyOption;
    int difficulty = 1;
    g_zOpt_GameDifficultyOption = &difficulty;
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        zUtil::ZRDR_PreallocNodePool(2);
    }

    const char *const dir = "zvehicle_select_zrd_by_difficulty_smoke_dir";
    CreateDirectoryA(dir, nullptr);

    char easyPath[MAX_PATH] = {};
    char hardPath[MAX_PATH] = {};
    std::sprintf(easyPath, "%s\\vehicle_easy.zrd", dir);
    std::sprintf(hardPath, "%s\\vehicle_hard.zrd", dir);
    std::remove(easyPath);
    std::remove(hardPath);

    std::FILE *file = std::fopen(easyPath, "wb");
    if (file == nullptr) {
        g_zOpt_GameDifficultyOption = oldDifficultyOption;
        RemoveDirectoryA(dir);
        return 1;
    }
    std::fclose(file);

    difficulty = 0;
    const bool easyOk =
        std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle_easy.zrd") == 0;

    difficulty = 2;
    const bool missingHardFallsBack =
        std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle.zrd") == 0;

    file = std::fopen(hardPath, "wb");
    if (file == nullptr) {
        std::remove(easyPath);
        g_zOpt_GameDifficultyOption = oldDifficultyOption;
        RemoveDirectoryA(dir);
        return 2;
    }
    std::fclose(file);

    const bool hardOk =
        std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle_hard.zrd") == 0;

    difficulty = 1;
    const bool defaultOk =
        std::strcmp(zVehicle::SelectZrdByDifficulty(dir), "vehicle.zrd") == 0;

    std::remove(easyPath);
    std::remove(hardPath);
    RemoveDirectoryA(dir);
    g_zOpt_GameDifficultyOption = oldDifficultyOption;
    return easyOk && missingHardFallsBack && hardOk && defaultOk ? 0 : 3;
}

extern "C" int player_get_aiv_zrd_path_smoke(void) {
    const char *const path = Player::GetAivZrdPath();
    return path != nullptr && std::strcmp(path, "aiv.zrd") == 0 ? 0 : 1;
}

extern "C" int player_extract_vehicle_name_from_aiv_name_smoke(void) {
    char out[64] = {};

    Player::ExtractVehicleNameFromAivName("tank_01", out);
    if (std::strcmp(out, "tank") != 0) {
        return 1;
    }

    Player::ExtractVehicleNameFromAivName("hover_alpha", out);
    if (std::strcmp(out, "hover_alpha") != 0) {
        return 2;
    }

    Player::ExtractVehicleNameFromAivName("_1", out);
    if (std::strcmp(out, "") != 0) {
        return 3;
    }

    Player::ExtractVehicleNameFromAivName("", out);
    return std::strcmp(out, "") == 0 ? 0 : 4;
}

extern "C" int player_los_from_fx_offset_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zClass_NodePartial *const oldMainCamera = g_MainCamera;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    PlayerProbeSampleCandidateBuffer *const oldPickBuffer = g_DiPickCandidateBuffer;
    const int oldBreakOnFirst = g_cls_di_BreakOnFirstCandidate;
    const int oldStopAfterFirst = g_cls_di_StopAfterFirstHit;

    zUtil_PlayerStateStorage playerState = {};
    zInput_GameStateOrMapTablePartial gameStateOrMap = {};
    zClass_NodePartial excludedNode = {};
    zClass_NodePartial rootNode = {};
    zClass_NodePartial worldNode = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    const zVec3 targetPoint = {10.0f, 20.0f, 30.0f};

    playerState.rootNode = &rootNode;
    playerState.fxOffsetWorld = {1.0f, 2.0f, 3.0f};
    playerState.variantTag.count = 2;
    playerState.variantTag.tags[0] = 7;
    playerState.variantTag.tags[1] = 8;
    playerState.variantTag.tags[2] = 0xff;
    gameStateOrMap.playerState =
        static_cast<zInput_PlayerStatePartial *>(static_cast<void *>(&playerState));
    worldNode.classData = &worldData;
    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = {4.0f, 5.0f, 6.0f};

    g_GameStateOrMapTable = &gameStateOrMap;
    g_Player_RuntimeDiScene = &worldNode;
    g_MainCamera = &cameraNode;
    g_cls_di_BreakOnFirstCandidate = 99;
    g_cls_di_StopAfterFirstHit = 99;

    const int forwardResult =
        AINet::HasLineOfSightFromLocalPlayerFxOffset(&excludedNode, &targetPoint, 1);
    const bool forwardOk =
        forwardResult == 1 && FloatNear(g_DiPickQueryPoint.x, 1.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 2.0f) && FloatNear(g_DiPickQueryPoint.z, 3.0f) &&
        FloatNear(g_DiSegmentEnd.x, 10.0f) && FloatNear(g_DiSegmentEnd.y, 20.0f) &&
        FloatNear(g_DiSegmentEnd.z, 30.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0 &&
        g_cls_di_BreakOnFirstCandidate == 0 && g_cls_di_StopAfterFirstHit == 0 &&
        g_Variant_CurrentTag.count == 2 && g_Variant_CurrentTag.tags[0] == 7 &&
        g_Variant_CurrentTag.tags[1] == 8;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int reverseResult =
        AINet::HasLineOfSightFromLocalPlayerFxOffset(&excludedNode, &targetPoint, 0);
    const bool reverseOk =
        reverseResult == 1 && FloatNear(g_DiPickQueryPoint.x, 10.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 20.0f) && FloatNear(g_DiPickQueryPoint.z, 30.0f) &&
        FloatNear(g_DiSegmentEnd.x, 1.0f) && FloatNear(g_DiSegmentEnd.y, 2.0f) &&
        FloatNear(g_DiSegmentEnd.z, 3.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int cameraForwardResult =
        AINet::HasLineOfSightFromCameraTarget(&excludedNode, &targetPoint, 1);
    const bool cameraForwardOk =
        cameraForwardResult == 1 && FloatNear(g_DiPickQueryPoint.x, 4.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 5.0f) && FloatNear(g_DiPickQueryPoint.z, 6.0f) &&
        FloatNear(g_DiSegmentEnd.x, 10.0f) && FloatNear(g_DiSegmentEnd.y, 20.0f) &&
        FloatNear(g_DiSegmentEnd.z, 30.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0 &&
        g_Variant_CurrentTag.count == 2 && g_Variant_CurrentTag.tags[0] == 7 &&
        g_Variant_CurrentTag.tags[1] == 8;

    excludedNode.flags = 0x10;
    rootNode.flags = 0x10;
    const int cameraReverseResult =
        AINet::HasLineOfSightFromCameraTarget(&excludedNode, &targetPoint, 0);
    const bool cameraReverseOk =
        cameraReverseResult == 1 && FloatNear(g_DiPickQueryPoint.x, 10.0f) &&
        FloatNear(g_DiPickQueryPoint.y, 20.0f) && FloatNear(g_DiPickQueryPoint.z, 30.0f) &&
        FloatNear(g_DiSegmentEnd.x, 4.0f) && FloatNear(g_DiSegmentEnd.y, 5.0f) &&
        FloatNear(g_DiSegmentEnd.z, 6.0f) &&
        (excludedNode.flags & 0x10) != 0 && (rootNode.flags & 0x10) != 0;

    g_cls_di_StopAfterFirstHit = oldStopAfterFirst;
    g_cls_di_BreakOnFirstCandidate = oldBreakOnFirst;
    g_DiPickCandidateBuffer = oldPickBuffer;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_MainCamera = oldMainCamera;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return forwardOk && reverseOk && cameraForwardOk && cameraReverseOk ? 0 : 1;
}
