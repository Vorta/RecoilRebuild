#include "Battlesport/GameNet.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/NetUi.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/mission.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/include/zClipRect.h"

#include <dplay.h>

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" NetPkt10_QSandEvent g_NetPkt10_QSandEventRelayBuf;

namespace {
RecoilNamedVtable *TestObjectVtable(void *object)
{
    return *(RecoilNamedVtable **)object;
}

bool TestMfcWindowConstructed(CWnd &wnd)
{
    return *(void **)&wnd != 0 && wnd.m_hWnd == 0;
}

int g_setSessionDescCalls;
std::int32_t g_setSessionDescResult;
int g_sendCalls;
std::uint32_t g_sendFlags;
void *g_sendPacket;
std::uint32_t g_sendPacketSize;
std::uint32_t g_sendPacketBytesSize;
unsigned char g_sendPacketBytes[0x200];
int g_chatComposeSetTextFmtCalls;
HudUiPanel *g_chatComposeSetTextFmtThis;
char g_chatComposeSetTextFmtText[32];
int g_remoteHudSetVisibleCount;
int g_remoteHudLastVisible;
int g_remoteHudSetPosCount;
HudUiPanel *g_remoteHudSetPosThis;
int g_remoteHudLastX;
int g_remoteHudLastY;
int g_spawnRemoteSetVisibleCount;
int g_spawnRemoteLastVisible;
int g_pkt14StateEnterCount;
int g_qsandRelayCallbackCount;
int g_qsandRelayCallbackResult;
int g_netSessionBrowserDtorStep;
int g_netSessionBrowserDtorOrder[7];
void *g_netSessionBrowserDtorThis[7];
int g_netSessionBrowserScalarDtorCalls;
void *g_netSessionBrowserScalarDtorThis[2];
int g_netSessionBrowserDdxStep;
int g_netSessionBrowserDdxKind[13];
int g_netSessionBrowserDdxIdOrLimit[13];
void *g_netSessionBrowserDdxContext[13];
void *g_netSessionBrowserDdxValue[13];
unsigned int g_netSessionBrowserDdxMin[13];
unsigned int g_netSessionBrowserDdxMax[13];
int g_netSessionBrowserOnDestroyStep;
void *g_netSessionBrowserOnDestroyThis;
HWND g_netSessionBrowserKillTimerHwnd;
UINT_PTR g_netSessionBrowserKillTimerId;
int g_netSessionConfigSendMessageStep;
HWND g_netSessionConfigSendMessageHwnd;
HWND g_netSessionConfigSendMessageHwndByCall[20];
UINT g_netSessionConfigSendMessageMsgByCall[20];
WPARAM g_netSessionConfigSendMessageWParamByCall[20];
LPARAM g_netSessionConfigSendMessageLParamByCall[20];
int g_netSessionConfigAddStringCalls;
LRESULT g_netSessionConfigSelectedComboIndex;
LRESULT g_netSessionConfigSelectedItemData;
WPARAM g_netSessionConfigItemDataWParam;
int g_netSessionConfigOnInitBaseCalls;
void *g_netSessionConfigOnInitBaseThis;
int g_netSessionConfigUpdateDataCalls;
BOOL g_netSessionConfigUpdateDataSave;
int g_netSessionConfigGetNetworkModemEnabledCalls;
int g_netSessionConfigGetNetworkModemEnabledResult;
int g_netSessionConfigSetDlgItemTextCalls;
void *g_netSessionConfigSetDlgItemTextThis;
int g_netSessionConfigSetDlgItemTextId;
const char *g_netSessionConfigSetDlgItemTextValue;
UINT_PTR g_netSessionBrowserHelpFindExecutableResult;
int g_netSessionBrowserHelpFindExecutableCalls;
bool g_netSessionBrowserHelpFindExecutableArgsOk;
int g_netSessionBrowserHelpShellExecuteCalls;
bool g_netSessionBrowserHelpShellExecuteArgsOk;
int g_netSessionBrowserHelpMessageBoxCalls;
bool g_netSessionBrowserHelpMessageBoxArgsOk;
void *g_netSessionBrowserHelpMessageBoxThis;
const char *g_netSessionBrowserHelpExpectedMessageText;
int g_netSessionBrowserRefreshEnumSessionsCalls;
int g_netSessionBrowserRefreshEnumSessionsResult;
int g_netSessionBrowserRefreshSendMessageStep;
int g_netSessionBrowserRefreshAddedCount;
char g_netSessionBrowserRefreshRows[4][128];
int g_netSessionBrowserRefreshItemData[4];
int g_netSessionBrowserRefreshSelectedIndex;
bool g_netSessionBrowserRefreshResetSeen;
bool g_netSessionBrowserRefreshMessagesOk;
int g_netSessionBrowserRefreshFormatCalls;
unsigned int g_netSessionBrowserRefreshFormatMessageIds[4];
int g_netSessionBrowserRefreshFormatMaxChars[4];
int g_netSessionBrowserOnTimerRefreshCalls;
void *g_netSessionBrowserOnTimerRefreshThis;
int g_netSessionBrowserOnTimerDefaultCalls;
void *g_netSessionBrowserOnTimerDefaultThis;
int g_netSessionBrowserValidateUpdateDataCalls;
BOOL g_netSessionBrowserValidateUpdateDataSaveValues[4];
void *g_netSessionBrowserValidateUpdateDataThis[4];
int g_netSessionBrowserValidateMessageBoxCalls;
bool g_netSessionBrowserValidateMessageBoxArgsOk;
void *g_netSessionBrowserValidateMessageBoxThis;
int g_netSessionBrowserValidateSetFocusCalls;
void *g_netSessionBrowserValidateSetFocusThis;
int g_netSessionBrowserOnCreateValidateCalls;
void *g_netSessionBrowserOnCreateValidateThis;
int g_netSessionBrowserOnCreateValidateResult;
int g_netSessionBrowserOnCreateKillTimerCalls;
HWND g_netSessionBrowserOnCreateKillTimerHwnd;
UINT_PTR g_netSessionBrowserOnCreateKillTimerId;
int g_netSessionBrowserOnCreateCreateSessionCalls;
zNetworkSessionDescStatusFields g_netSessionBrowserOnCreateStatusFields;
int g_netSessionBrowserOnCreateCreateSessionResult;
int g_netSessionBrowserOnCreateSetNetworkCalls;
int g_netSessionBrowserOnCreateSetNetworkValue;
int g_netSessionBrowserOnCreateSetModemCalls;
int g_netSessionBrowserOnCreateSetModemValue;
int g_netSessionBrowserOnCreateGetPlayerNameCalls;
int g_netSessionBrowserOnCreateCreateLocalPlayerCalls;
char *g_netSessionBrowserOnCreateCreateLocalPlayerName;
int g_netSessionBrowserOnCreateOnOkCalls;
void *g_netSessionBrowserOnCreateOnOkThis;
int g_netSessionBrowserOnOkValidateCalls;
void *g_netSessionBrowserOnOkValidateThis;
int g_netSessionBrowserOnOkValidateResult;
int g_netSessionBrowserOnOkSetModemCalls;
int g_netSessionBrowserOnOkSetModemValue;
int g_netSessionBrowserOnOkKillTimerCalls;
HWND g_netSessionBrowserOnOkKillTimerHwnd;
UINT_PTR g_netSessionBrowserOnOkKillTimerId;
int g_netSessionBrowserOnOkSendMessageCalls;
LRESULT g_netSessionBrowserOnOkListCurSel;
LRESULT g_netSessionBrowserOnOkListItemData;
WPARAM g_netSessionBrowserOnOkItemDataIndex;
int g_netSessionBrowserOnOkRefreshCalls;
void *g_netSessionBrowserOnOkRefreshThis;
int g_netSessionBrowserOnOkRefreshResult;
int g_netSessionBrowserOnOkOnOkCalls;
void *g_netSessionBrowserOnOkOnOkThis;
int g_netUiWsaStartupCalls;
WORD g_netUiWsaStartupVersion;
int g_netUiWsaStartupResult;
WORD g_netUiWsaStartupHighVersion;
int g_netUiWsaCleanupCalls;
int g_netUiMessageBeepCalls;
UINT g_netUiMessageBeepType;
int g_netUiGetFocusCalls;
HWND g_netUiGetFocusResult;
int g_netUiMessageBoxCalls;
HWND g_netUiMessageBoxHwnd;
char g_netUiMessageBoxText[256];
char g_netUiMessageBoxCaption[64];
UINT g_netUiMessageBoxType;
int g_netUiMessageBoxResult;
int g_dplayCreateCoInitializeCalls;
void *g_dplayCreateCoInitializeReserved;
int g_dplayCreateCoCreateCalls;
bool g_dplayCreateCoCreateArgsOk;
HRESULT g_dplayCreateCoCreateResult;
zNetwork_DPlay4 *g_dplayCreateCoCreateOut;
void *g_dplayCreateCoCreateInitialOut;
bool g_dplayCreateCoCreateWriteOut;
int g_dplayCreateReportErrorCalls;
int g_dplayCreateReportErrorHresult;
int g_dplayCreateReportErrorLine;
int g_dplayCreateReportOldCalls;
int g_dplayCreateReportOldFlags;
int g_dplayCreateReportOldLine;
char g_dplayCreateReportOldMessage[64];
int g_dplaySelectCloseCalls;
zNetwork_DPlay4 *g_dplaySelectCloseArg;
int g_dplaySelectCreateCalls;
zNetwork_DPlay4 **g_dplaySelectCreateOutPtr;
int g_dplaySelectCreateResult;
zNetwork_DPlay4 *g_dplaySelectCreateOut;
int g_dplaySelectInitializeCalls;
zNetworkDPlayServiceProviderInfo *g_dplaySelectInitializeProvider;
int g_dplaySelectInitializeResult;
int g_dplaySelectReportErrorCalls;
int g_dplaySelectReportErrorHresult;
int g_dplaySelectReportErrorLine;
int g_dplaySelectReportErrorResult;
int g_netSessionBrowserConnectKillTimerCalls;
HWND g_netSessionBrowserConnectKillTimerHwnd;
UINT_PTR g_netSessionBrowserConnectKillTimerId;
int g_netSessionBrowserConnectSendMessageCalls;
LRESULT g_netSessionBrowserConnectSelectedIndex;
zNetworkDPlayServiceProviderInfo *g_netSessionBrowserConnectItemData;
int g_netSessionBrowserConnectSetCurSelCalls;
WPARAM g_netSessionBrowserConnectSetCurSelWParam;
int g_netSessionBrowserConnectListResetCalls;
int g_netSessionBrowserConnectEnableCalls;
void *g_netSessionBrowserConnectEnableThis[8];
BOOL g_netSessionBrowserConnectEnableValue[8];
int g_netSessionBrowserConnectSetTextCalls;
void *g_netSessionBrowserConnectSetTextThis[8];
char g_netSessionBrowserConnectSetTextValue[8][64];
int g_netSessionBrowserConnectSetTimerCalls;
HWND g_netSessionBrowserConnectSetTimerHwnd;
UINT_PTR g_netSessionBrowserConnectSetTimerId;
UINT g_netSessionBrowserConnectSetTimerMs;
int g_netSessionBrowserConnectVerifyCalls;
char g_netSessionBrowserConnectVerifyCaption[64];
char g_netSessionBrowserConnectVerifyFormat[64];
int g_netSessionBrowserConnectVerifyResult;
int g_netSessionBrowserConnectSelectCalls;
zNetworkDPlayServiceProviderInfo *g_netSessionBrowserConnectSelectProvider;
int g_netSessionBrowserConnectRefreshCalls;
void *g_netSessionBrowserConnectRefreshThis;
int g_netSessionBrowserConnectRefreshResult;
int g_netSessionBrowserOnInitBaseCalls;
void *g_netSessionBrowserOnInitBaseThis;
int g_netSessionBrowserOnInitRefreshCalls;
zNetworkServiceProviderListVec *g_netSessionBrowserOnInitProviderList;
int g_netSessionBrowserOnInitSendMessageCalls;
int g_netSessionBrowserOnInitAddStringCalls;
char g_netSessionBrowserOnInitAddStringText[8][64];
int g_netSessionBrowserOnInitSetItemDataCalls;
WPARAM g_netSessionBrowserOnInitSetItemDataIndex[8];
LPARAM g_netSessionBrowserOnInitSetItemDataValue[8];
int g_netSessionBrowserOnInitSetCurSelCalls;
WPARAM g_netSessionBrowserOnInitSetCurSelIndex;
int g_netSessionBrowserOnInitSetTextCalls;
char g_netSessionBrowserOnInitSetTextValue[64];
int g_netSessionBrowserOnInitUpdateDataCalls;
BOOL g_netSessionBrowserOnInitUpdateDataSave;
char g_netSessionBrowserOnInitPlayerName[] = "Pilot";
int g_netSessionConfigAtexitCalls;
void(*g_netSessionConfigAtexitCallback)(void);
int g_netSessionConfigAtexitResult;

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct ScoreboardPacket2 {
    zNetworkPacketHeader header;
    std::int32_t entryCount;
    NetPkt09_PlayerScoreboardEntry entries[2];
};

template <typename T> T &FieldAt(void *object, std::size_t offset) {
    return *reinterpret_cast<T *>(static_cast<unsigned char *>(object) + offset);
}

void MakeGameNetReaderFloatNode(zReader::Node &node, float value) {
    node.type = zReader::ZRDR_NODE_FLOAT;
    node.value.f32 = value;
}

void MakeGameNetReaderArrayNode(zReader::Node &node, zReader::Node *payload, int count) {
    payload[0].type = zReader::ZRDR_NODE_INT;
    payload[0].value.i32 = count;
    node.type = zReader::ZRDR_NODE_ARRAY;
    node.value.nodes = payload;
}

bool WriteGameNetZrdU32(std::FILE *file, unsigned int value) {
    return std::fwrite(&value, sizeof(value), 1, file) == 1;
}

bool WriteGameNetZrdNode(std::FILE *file, const zReader::Node &node) {
    if (!WriteGameNetZrdU32(file, static_cast<unsigned int>(node.type))) {
        return false;
    }

    switch (node.type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        return WriteGameNetZrdU32(file, node.value.u32);
    case zReader::ZRDR_NODE_STRING: {
        const unsigned int length = static_cast<unsigned int>(std::strlen(node.value.str));
        return WriteGameNetZrdU32(file, length) &&
               std::fwrite(node.value.str, 1, length, file) == length;
    }
    case zReader::ZRDR_NODE_ARRAY: {
        const int count = node.value.nodes[0].value.i32;
        if (!WriteGameNetZrdU32(file, static_cast<unsigned int>(count))) {
            return false;
        }
        for (int index = 1; index < count; ++index) {
            if (!WriteGameNetZrdNode(file, node.value.nodes[index])) {
                return false;
            }
        }
        return true;
    }
    }

    return false;
}

bool WriteGameNetZrdFile(const char *path, const zReader::Node &root) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    const bool ok = WriteGameNetZrdNode(file, root);
    return std::fclose(file) == 0 && ok;
}

struct GameNetZrdArchiveEntry {
    const char *name;
    const zReader::Node *root;
};

bool MountGameNetZrdArchive(const char *path, const GameNetZrdArchiveEntry *entries,
                            int entryCount, zIndexArchive &archive, zZarFileRecord *records,
                            zArchiveListNode &archiveNode, zArchiveList &archiveList) {
    std::FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    bool ok = true;
    for (int index = 0; index < entryCount; ++index) {
        const long offset = std::ftell(file);
        if (offset < 0 || !WriteGameNetZrdNode(file, *entries[index].root)) {
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
    archive.hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
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

template <typename Method> unsigned int MethodAddress(Method method) {
    union {
        Method method;
        unsigned int address;
    } value = {method};
    return value.address;
}

struct TestPkt14AppState : RecoilApp_IState {
    void OnEnter() {
        ++g_pkt14StateEnterCount;
    }
};

RecoilApp_IState_Vtbl MakePkt14StateVtable() {
    RecoilApp_IState_Vtbl vtable{};
    vtable.OnEnter = MethodAddress(&TestPkt14AppState::OnEnter);
    return vtable;
}

RecoilApp_IState_Vtbl g_pkt14StateVtable = MakePkt14StateVtable();

void CleanupSingleQueuedItem(RecoilApp_StateQueue &queue) {
    const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
    RecoilPtr32 *const slot = reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
    RecoilApp_StateQueueItem *const item =
        reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
    RecoilPtr32 *const chunkList =
        reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(queue.m_chunkPtrList));
    void *const chunk = reinterpret_cast<void *>(static_cast<std::uintptr_t>(chunkList[1]));
    ::operator delete(item);
    ::operator delete(chunk);
    ::operator delete(chunkList);
}

bool FloatNear(float actual, float expected) {
    return actual >= expected - 0.0001f && actual <= expected + 0.0001f;
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return FloatNear(value.x, expected.x) && FloatNear(value.y, expected.y) &&
           FloatNear(value.z, expected.z);
}

bool PatchImportByOrdinal(const char *dllName, WORD ordinal, void *replacement,
                          ImportFunctionPatch &patch) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    IMAGE_DOS_HEADER *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll =
            reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) ||
                static_cast<WORD>(nameThunk->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = static_cast<ULONG_PTR>(reinterpret_cast<std::uintptr_t>(replacement));
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchImportByName(const char *dllName, const char *functionName, void *replacement,
                       ImportFunctionPatch &patch) {
    unsigned char *const imageBase = reinterpret_cast<unsigned char *>(GetModuleHandleA(nullptr));
    IMAGE_DOS_HEADER *const dos = reinterpret_cast<IMAGE_DOS_HEADER *>(imageBase);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = reinterpret_cast<IMAGE_NT_HEADERS *>(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR *>(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll =
            reinterpret_cast<const char *>(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = reinterpret_cast<IMAGE_THUNK_DATA *>(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            reinterpret_cast<IMAGE_THUNK_DATA *>(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                reinterpret_cast<IMAGE_IMPORT_BY_NAME *>(
                    imageBase + nameThunk->u1.AddressOfData);
            if (std::strcmp(reinterpret_cast<const char *>(importName->Name),
                            functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = static_cast<ULONG_PTR>(reinterpret_cast<std::uintptr_t>(replacement));
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = nullptr;
    patch.original = 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    if (target == nullptr) {
        patch.address = nullptr;
        return false;
    }

    patch.address = reinterpret_cast<unsigned char *>(target);
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
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
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = nullptr;
}

void *NetSessionBrowserRefreshSessionListAddress() {
    int (NetSessionBrowserDialog::*method)() =
        &NetSessionBrowserDialog::RefreshSessionList;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

void *NetSessionBrowserValidatePlayerNameAddress() {
    int (NetSessionBrowserDialog::*method)() =
        &NetSessionBrowserDialog::ValidatePlayerName;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

void *NetSessionBrowserDestructorAddress() {
    void (NetSessionBrowserDialog::*method)() =
        &NetSessionBrowserDialog::Destructor;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

void *CWndUpdateDataAddress() {
    int (CWnd::*method)(BOOL) = &CWnd::UpdateData;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

void *CWndEnableWindowAddress() {
    BOOL (CWnd::*method)(BOOL) = &CWnd::EnableWindow;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

void *CWndSetWindowTextAAddress() {
    void (CWnd::*method)(LPCSTR) = &CWnd::SetWindowTextA;
    void *address = nullptr;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

template <typename Method>
ULONG_PTR MemberPointerBits(Method method) {
    ULONG_PTR bits = 0;
    std::memcpy(&bits, &method, sizeof(method));
    return bits;
}

ULONG_PTR MsgMapEntryHandlerBits(const AFX_MSGMAP_ENTRY &entry) {
    ULONG_PTR bits = 0;
    std::memcpy(&bits, &entry.pfn, sizeof(entry.pfn));
    return bits;
}

void DeleteTopMessageStackFonts(HudUiTopMessageStack &stack) {
    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&stack.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }
}

std::int32_t __stdcall SetSessionDescFake(zNetwork_DPlay4 *, zNetworkDPlaySessionDesc *,
                                               std::uint32_t) {
    ++g_setSessionDescCalls;
    return g_setSessionDescResult;
}

std::int32_t __stdcall SendFake(zNetwork_DPlay4 *, std::uint32_t, std::uint32_t,
                                     std::uint32_t flags, void *packet,
                                     std::uint32_t packetSizeBytes) {
    ++g_sendCalls;
    g_sendFlags = flags;
    g_sendPacket = packet;
    g_sendPacketSize = packetSizeBytes;
    g_sendPacketBytesSize = packetSizeBytes;
    if (packetSizeBytes <= sizeof(g_sendPacketBytes)) {
        std::memcpy(g_sendPacketBytes, packet, packetSizeBytes);
    }
    return 0;
}

void ChatComposeSetTextFmtFake(HudUiPanel *self, const char *format, ...) {
    ++g_chatComposeSetTextFmtCalls;
    g_chatComposeSetTextFmtThis = self;
    std::strncpy(g_chatComposeSetTextFmtText, format != nullptr ? format : "", sizeof(g_chatComposeSetTextFmtText));
    g_chatComposeSetTextFmtText[sizeof(g_chatComposeSetTextFmtText) - 1] = '\0';
}

int __fastcall QSandRelayCallbackFake(void *) {
    ++g_qsandRelayCallbackCount;
    return g_qsandRelayCallbackResult;
}

void RecordNetSessionBrowserDtor(void *self, int stepId) {
    const int index = g_netSessionBrowserDtorStep;
    if (index < 7) {
        g_netSessionBrowserDtorOrder[index] = stepId;
        g_netSessionBrowserDtorThis[index] = self;
    }
    ++g_netSessionBrowserDtorStep;
}

void __fastcall FakeNetSessionBrowserCStringDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 1);
}

void __fastcall FakeNetSessionBrowserComboDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 2);
}

void __fastcall FakeNetSessionConfigSpinDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 3);
}

void __fastcall FakeNetSessionBrowserListDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 3);
}

void __fastcall FakeNetSessionBrowserCreateButtonDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 4);
}

void __fastcall FakeNetSessionBrowserEditDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 6);
}

void __fastcall FakeNetSessionBrowserDialogDtor(void *self) {
    RecordNetSessionBrowserDtor(self, 7);
}

void __fastcall FakeNetSessionBrowserDestructor(NetSessionBrowserDialog *self) {
    const int index = g_netSessionBrowserScalarDtorCalls;
    if (index < 2) {
        g_netSessionBrowserScalarDtorThis[index] = self;
    }
    ++g_netSessionBrowserScalarDtorCalls;
}

int FakeNetSessionConfigAtexit(void(*callback)(void)) {
    ++g_netSessionConfigAtexitCalls;
    g_netSessionConfigAtexitCallback = callback;
    return g_netSessionConfigAtexitResult;
}

BOOL __fastcall FakeNetSessionConfigOnInitBase(void *self) {
    ++g_netSessionConfigOnInitBaseCalls;
    g_netSessionConfigOnInitBaseThis = self;
    return TRUE;
}

int __fastcall FakeNetSessionConfigUpdateData(void *, void *,
                                                  BOOL saveAndValidate) {
    ++g_netSessionConfigUpdateDataCalls;
    g_netSessionConfigUpdateDataSave = saveAndValidate;
    return 1;
}

int FakeNetSessionConfigGetNetworkModemEnabled() {
    ++g_netSessionConfigGetNetworkModemEnabledCalls;
    return g_netSessionConfigGetNetworkModemEnabledResult;
}

void RecordNetSessionBrowserDdx(CDataExchange *dataExchange, int kind,
                                int idOrLimit, void *value) {
    const int index = g_netSessionBrowserDdxStep;
    if (index < 13) {
        g_netSessionBrowserDdxKind[index] = kind;
        g_netSessionBrowserDdxIdOrLimit[index] = idOrLimit;
        g_netSessionBrowserDdxContext[index] = dataExchange;
        g_netSessionBrowserDdxValue[index] = value;
        g_netSessionBrowserDdxMin[index] = 0;
        g_netSessionBrowserDdxMax[index] = 0;
    }
    ++g_netSessionBrowserDdxStep;
}

void __stdcall FakeNetSessionBrowserDDXControl(CDataExchange *dataExchange,
                                                   int controlId,
                                                   void *control) {
    RecordNetSessionBrowserDdx(dataExchange, 1, controlId, control);
}

void __stdcall FakeNetSessionBrowserDDXText(CDataExchange *dataExchange,
                                                int controlId,
                                                CString *value) {
    RecordNetSessionBrowserDdx(dataExchange, 2, controlId, value);
}

void __stdcall FakeNetSessionBrowserDDVMaxChars(CDataExchange *dataExchange,
                                                    CString *value,
                                                    int maxChars) {
    RecordNetSessionBrowserDdx(dataExchange, 3, maxChars, value);
}

void __stdcall FakeNetSessionConfigDDXTextUInt(CDataExchange *dataExchange,
                                                   int controlId,
                                                   unsigned int *value) {
    RecordNetSessionBrowserDdx(dataExchange, 4, controlId, value);
}

void __stdcall FakeNetSessionConfigDDVMinMaxUInt(
    CDataExchange *dataExchange,
    unsigned int value,
    unsigned int minValue,
    unsigned int maxValue) {
    RecordNetSessionBrowserDdx(dataExchange, 5, (int)value, 0);
    const int index = g_netSessionBrowserDdxStep - 1;
    if (index >= 0 && index < 13) {
        g_netSessionBrowserDdxMin[index] = minValue;
        g_netSessionBrowserDdxMax[index] = maxValue;
    }
}

void __stdcall FakeNetSessionConfigDDXCheck(CDataExchange *dataExchange,
                                                int controlId,
                                                int *value) {
    RecordNetSessionBrowserDdx(dataExchange, 6, controlId, value);
}

void __fastcall FakeNetSessionBrowserCWndOnDestroy(void *self, void *) {
    ++g_netSessionBrowserOnDestroyStep;
    g_netSessionBrowserOnDestroyThis = self;
}

BOOL WINAPI FakeNetSessionBrowserKillTimer(HWND hwnd, UINT_PTR timerId) {
    if (g_netSessionBrowserOnDestroyStep == 1) {
        ++g_netSessionBrowserOnDestroyStep;
    } else {
        g_netSessionBrowserOnDestroyStep = -100;
    }
    g_netSessionBrowserKillTimerHwnd = hwnd;
    g_netSessionBrowserKillTimerId = timerId;
    return TRUE;
}

LRESULT WINAPI FakeNetSessionConfigSendMessageA(HWND hwnd, UINT message,
                                                WPARAM wParam, LPARAM lParam) {
    const int callIndex = g_netSessionConfigSendMessageStep;
    if (callIndex < 20) {
        g_netSessionConfigSendMessageHwndByCall[callIndex] = hwnd;
        g_netSessionConfigSendMessageMsgByCall[callIndex] = message;
        g_netSessionConfigSendMessageWParamByCall[callIndex] = wParam;
        g_netSessionConfigSendMessageLParamByCall[callIndex] = lParam;
    }
    ++g_netSessionConfigSendMessageStep;
    g_netSessionConfigSendMessageHwnd = hwnd;
    if (message == CB_ADDSTRING) {
        return g_netSessionConfigAddStringCalls++;
    }
    if (message == CB_GETCURSEL && wParam == 0 && lParam == 0) {
        return g_netSessionConfigSelectedComboIndex;
    }
    if (message == CB_GETITEMDATA && lParam == 0) {
        g_netSessionConfigItemDataWParam = wParam;
        return g_netSessionConfigSelectedItemData;
    }
    return 0;
}

void ResetNetSessionConfigOnInitLog(int modemEnabled) {
    g_netSessionConfigSendMessageStep = 0;
    g_netSessionConfigSendMessageHwnd = nullptr;
    std::memset(g_netSessionConfigSendMessageHwndByCall, 0,
                sizeof(g_netSessionConfigSendMessageHwndByCall));
    std::memset(g_netSessionConfigSendMessageMsgByCall, 0,
                sizeof(g_netSessionConfigSendMessageMsgByCall));
    std::memset(g_netSessionConfigSendMessageWParamByCall, 0,
                sizeof(g_netSessionConfigSendMessageWParamByCall));
    std::memset(g_netSessionConfigSendMessageLParamByCall, 0,
                sizeof(g_netSessionConfigSendMessageLParamByCall));
    g_netSessionConfigAddStringCalls = 0;
    g_netSessionConfigOnInitBaseCalls = 0;
    g_netSessionConfigOnInitBaseThis = nullptr;
    g_netSessionConfigUpdateDataCalls = 0;
    g_netSessionConfigUpdateDataSave = TRUE;
    g_netSessionConfigGetNetworkModemEnabledCalls = 0;
    g_netSessionConfigGetNetworkModemEnabledResult = modemEnabled;
    g_netSessionConfigSetDlgItemTextCalls = 0;
    g_netSessionConfigSetDlgItemTextThis = nullptr;
    g_netSessionConfigSetDlgItemTextId = 0;
    g_netSessionConfigSetDlgItemTextValue = nullptr;
}

char *__fastcall FakeNetSessionBrowserHelpGetMessageString(unsigned int messageId) {
    switch (messageId) {
    case 25:
        return const_cast<char *>("Help");
    case 24:
        return const_cast<char *>("Player Name");
    case 23:
        return const_cast<char *>("Enter a player name.");
    case 18:
        return const_cast<char *>("Network");
    case 38:
        return const_cast<char *>("Need Winsock %u.%u");
    case 32:
        return const_cast<char *>("No file association");
    case 33:
        return const_cast<char *>("No DDE association");
    case 34:
        return const_cast<char *>("File not found");
    case 36:
        return const_cast<char *>("Association incomplete");
    case 53:
        return const_cast<char *>("Dial");
    case 54:
        return const_cast<char *>("Host Modem");
    case 55:
        return const_cast<char *>("Join");
    case 56:
        return const_cast<char *>("Refresh");
    case 273:
        return const_cast<char *>("No providers");
    case 12352:
        return const_cast<char *>("Max teams");
    case 12353:
        return const_cast<char *>("Max players");
    default:
        return const_cast<char *>("");
    }
}

void __fastcall FakeNetSessionConfigSetDlgItemTextA(
    void *self,
    void *,
    int controlId,
    LPCSTR text) {
    ++g_netSessionConfigSetDlgItemTextCalls;
    g_netSessionConfigSetDlgItemTextThis = self;
    g_netSessionConfigSetDlgItemTextId = controlId;
    g_netSessionConfigSetDlgItemTextValue = text;
}

HINSTANCE WINAPI FakeNetSessionBrowserHelpFindExecutableA(LPCSTR file,
                                                          LPCSTR directory,
                                                          LPSTR result) {
    ++g_netSessionBrowserHelpFindExecutableCalls;
    g_netSessionBrowserHelpFindExecutableArgsOk =
        file != nullptr && std::strcmp(file, "Docs\\Index.html") == 0 &&
        directory == nullptr && result != nullptr;
    if (result != nullptr) {
        strcpy_s(result, 256, "fake-browser.exe");
    }

    return reinterpret_cast<HINSTANCE>(g_netSessionBrowserHelpFindExecutableResult);
}

HINSTANCE WINAPI FakeNetSessionBrowserHelpShellExecuteA(HWND hwnd,
                                                       LPCSTR operation,
                                                       LPCSTR file,
                                                       LPCSTR parameters,
                                                       LPCSTR directory,
                                                       INT showCommand) {
    ++g_netSessionBrowserHelpShellExecuteCalls;
    g_netSessionBrowserHelpShellExecuteArgsOk =
        hwnd == g_RecoilApp_hWndMain && operation != nullptr &&
        std::strcmp(operation, "open") == 0 && file != nullptr &&
        std::strcmp(file, "Docs\\Index.html") == 0 && parameters == nullptr &&
        directory == nullptr && showCommand == SW_HIDE;
    return reinterpret_cast<HINSTANCE>(33);
}

int __fastcall FakeNetSessionBrowserHelpMessageBoxA(void *self, void *,
                                                        LPCSTR text,
                                                        LPCSTR caption,
                                                        UINT type) {
    ++g_netSessionBrowserHelpMessageBoxCalls;
    g_netSessionBrowserHelpMessageBoxThis = self;
    g_netSessionBrowserHelpMessageBoxArgsOk =
        text == g_netSessionBrowserHelpExpectedMessageText &&
        caption != nullptr && std::strcmp(caption, "Help") == 0 &&
        type == MB_ICONEXCLAMATION;
    return IDOK;
}

int FakeNetSessionBrowserRefreshEnumSessions() {
    ++g_netSessionBrowserRefreshEnumSessionsCalls;
    return g_netSessionBrowserRefreshEnumSessionsResult;
}

LRESULT WINAPI FakeNetSessionBrowserRefreshSendMessageA(HWND hwnd, UINT message,
                                                        WPARAM wParam, LPARAM lParam) {
    ++g_netSessionBrowserRefreshSendMessageStep;
    if (hwnd != reinterpret_cast<HWND>(0x5150)) {
        g_netSessionBrowserRefreshMessagesOk = false;
    }

    switch (message) {
    case LB_GETCURSEL:
        return LB_ERR;

    case LB_RESETCONTENT:
        g_netSessionBrowserRefreshResetSeen = true;
        g_netSessionBrowserRefreshAddedCount = 0;
        return 0;

    case LB_ADDSTRING: {
        const int index = g_netSessionBrowserRefreshAddedCount++;
        if (index < 4 && lParam != 0) {
            std::strncpy(g_netSessionBrowserRefreshRows[index],
                         reinterpret_cast<const char *>(lParam),
                         sizeof(g_netSessionBrowserRefreshRows[index]));
            g_netSessionBrowserRefreshRows[index]
                                          [sizeof(g_netSessionBrowserRefreshRows[index]) - 1] = '\0';
        }
        return index;
    }

    case LB_SETITEMDATA:
        if (wParam < 4) {
            g_netSessionBrowserRefreshItemData[wParam] = static_cast<int>(lParam);
        }
        return 0;

    case LB_SETCURSEL:
        g_netSessionBrowserRefreshSelectedIndex = static_cast<int>(wParam);
        return wParam;
    }

    g_netSessionBrowserRefreshMessagesOk = false;
    return 0;
}

unsigned int FakeNetSessionBrowserRefreshFormatMessage(
    char *outBuffer, int maxChars, unsigned int messageId, ...) {
    const int index = g_netSessionBrowserRefreshFormatCalls++;
    if (index < 4) {
        g_netSessionBrowserRefreshFormatMessageIds[index] = messageId;
        g_netSessionBrowserRefreshFormatMaxChars[index] = maxChars;
    }

    va_list args;
    va_start(args, messageId);
    const char *sessionName = va_arg(args, const char *);
    const int maxPlayers = va_arg(args, int);
    const int currentPlayers = va_arg(args, int);
    va_end(args);

    std::snprintf(outBuffer, static_cast<size_t>(maxChars), "%s:%d/%d", sessionName,
                  currentPlayers, maxPlayers);
    return static_cast<unsigned int>(std::strlen(outBuffer));
}

int __fastcall FakeNetSessionBrowserRefreshSessionList(void *self) {
    ++g_netSessionBrowserOnTimerRefreshCalls;
    g_netSessionBrowserOnTimerRefreshThis = self;
    return 3;
}

long __fastcall FakeNetSessionBrowserCWndDefault(void *self) {
    ++g_netSessionBrowserOnTimerDefaultCalls;
    g_netSessionBrowserOnTimerDefaultThis = self;
    return 77;
}

int __fastcall FakeNetSessionBrowserValidateUpdateData(void *self, void *,
                                                           BOOL saveAndValidate) {
    const int index = g_netSessionBrowserValidateUpdateDataCalls;
    if (index < 4) {
        g_netSessionBrowserValidateUpdateDataSaveValues[index] = saveAndValidate;
        g_netSessionBrowserValidateUpdateDataThis[index] = self;
    }
    ++g_netSessionBrowserValidateUpdateDataCalls;
    return 1;
}

int __fastcall FakeNetSessionBrowserValidateMessageBoxA(void *self, void *,
                                                            LPCSTR text,
                                                            LPCSTR caption,
                                                            UINT type) {
    ++g_netSessionBrowserValidateMessageBoxCalls;
    g_netSessionBrowserValidateMessageBoxThis = self;
    g_netSessionBrowserValidateMessageBoxArgsOk =
        text != nullptr && std::strcmp(text, "Enter a player name.") == 0 &&
        caption != nullptr && std::strcmp(caption, "Player Name") == 0 &&
        type == MB_ICONHAND;
    return IDOK;
}

void *__fastcall FakeNetSessionBrowserValidateSetFocus(void *self) {
    ++g_netSessionBrowserValidateSetFocusCalls;
    g_netSessionBrowserValidateSetFocusThis = self;
    return self;
}

void ResetNetSessionBrowserValidateLog() {
    g_netSessionBrowserValidateUpdateDataCalls = 0;
    g_netSessionBrowserValidateMessageBoxCalls = 0;
    g_netSessionBrowserValidateMessageBoxArgsOk = false;
    g_netSessionBrowserValidateMessageBoxThis = nullptr;
    g_netSessionBrowserValidateSetFocusCalls = 0;
    g_netSessionBrowserValidateSetFocusThis = nullptr;
    for (int index = 0; index < 4; ++index) {
        g_netSessionBrowserValidateUpdateDataSaveValues[index] = FALSE;
        g_netSessionBrowserValidateUpdateDataThis[index] = nullptr;
    }
}

int __fastcall FakeNetSessionBrowserOnCreateValidate(void *self) {
    ++g_netSessionBrowserOnCreateValidateCalls;
    g_netSessionBrowserOnCreateValidateThis = self;
    return g_netSessionBrowserOnCreateValidateResult;
}

BOOL WINAPI FakeNetSessionBrowserOnCreateKillTimer(HWND hwnd, UINT_PTR timerId) {
    ++g_netSessionBrowserOnCreateKillTimerCalls;
    g_netSessionBrowserOnCreateKillTimerHwnd = hwnd;
    g_netSessionBrowserOnCreateKillTimerId = timerId;
    return TRUE;
}

int __fastcall
FakeNetSessionBrowserOnCreateCreateSession(zNetworkSessionDescStatusFields *statusFields) {
    ++g_netSessionBrowserOnCreateCreateSessionCalls;
    if (statusFields != nullptr) {
        g_netSessionBrowserOnCreateStatusFields = *statusFields;
    }
    return g_netSessionBrowserOnCreateCreateSessionResult;
}

void __fastcall FakeNetSessionBrowserOnCreateSetNetworkEnabled(int value) {
    ++g_netSessionBrowserOnCreateSetNetworkCalls;
    g_netSessionBrowserOnCreateSetNetworkValue = value;
}

void __fastcall FakeNetSessionBrowserOnCreateSetModemEnabled(int value) {
    ++g_netSessionBrowserOnCreateSetModemCalls;
    g_netSessionBrowserOnCreateSetModemValue = value;
}

char *FakeNetSessionBrowserOnCreateGetPlayerName() {
    static char playerName[] = "Pilot";
    ++g_netSessionBrowserOnCreateGetPlayerNameCalls;
    return playerName;
}

int __fastcall FakeNetSessionBrowserOnCreateCreateLocalPlayer(char *playerName) {
    ++g_netSessionBrowserOnCreateCreateLocalPlayerCalls;
    g_netSessionBrowserOnCreateCreateLocalPlayerName = playerName;
    return 77;
}

void __fastcall FakeNetSessionBrowserOnCreateOnOK(void *self) {
    ++g_netSessionBrowserOnCreateOnOkCalls;
    g_netSessionBrowserOnCreateOnOkThis = self;
}

void ResetNetSessionBrowserOnCreateLog() {
    g_netSessionBrowserOnCreateValidateCalls = 0;
    g_netSessionBrowserOnCreateValidateThis = nullptr;
    g_netSessionBrowserOnCreateKillTimerCalls = 0;
    g_netSessionBrowserOnCreateKillTimerHwnd = nullptr;
    g_netSessionBrowserOnCreateKillTimerId = 0;
    g_netSessionBrowserOnCreateCreateSessionCalls = 0;
    g_netSessionBrowserOnCreateStatusFields = {};
    g_netSessionBrowserOnCreateSetNetworkCalls = 0;
    g_netSessionBrowserOnCreateSetNetworkValue = 0;
    g_netSessionBrowserOnCreateSetModemCalls = 0;
    g_netSessionBrowserOnCreateSetModemValue = 0;
    g_netSessionBrowserOnCreateGetPlayerNameCalls = 0;
    g_netSessionBrowserOnCreateCreateLocalPlayerCalls = 0;
    g_netSessionBrowserOnCreateCreateLocalPlayerName = nullptr;
    g_netSessionBrowserOnCreateOnOkCalls = 0;
    g_netSessionBrowserOnCreateOnOkThis = nullptr;
}

int __fastcall FakeNetSessionBrowserOnOkValidate(void *self) {
    ++g_netSessionBrowserOnOkValidateCalls;
    g_netSessionBrowserOnOkValidateThis = self;
    return g_netSessionBrowserOnOkValidateResult;
}

void __fastcall FakeNetSessionBrowserOnOkSetModemEnabled(int value) {
    ++g_netSessionBrowserOnOkSetModemCalls;
    g_netSessionBrowserOnOkSetModemValue = value;
}

BOOL WINAPI FakeNetSessionBrowserOnOkKillTimer(HWND hwnd, UINT_PTR timerId) {
    ++g_netSessionBrowserOnOkKillTimerCalls;
    g_netSessionBrowserOnOkKillTimerHwnd = hwnd;
    g_netSessionBrowserOnOkKillTimerId = timerId;
    return TRUE;
}

LRESULT WINAPI FakeNetSessionBrowserOnOkSendMessageA(HWND, UINT message,
                                                     WPARAM wParam, LPARAM) {
    ++g_netSessionBrowserOnOkSendMessageCalls;
    if (message == LB_GETCURSEL) {
        return g_netSessionBrowserOnOkListCurSel;
    }
    if (message == LB_GETITEMDATA) {
        g_netSessionBrowserOnOkItemDataIndex = wParam;
        return g_netSessionBrowserOnOkListItemData;
    }
    return 0;
}

int __fastcall FakeNetSessionBrowserOnOkRefreshSessionList(void *self) {
    ++g_netSessionBrowserOnOkRefreshCalls;
    g_netSessionBrowserOnOkRefreshThis = self;
    return g_netSessionBrowserOnOkRefreshResult;
}

void __fastcall FakeNetSessionBrowserOnOkOnOK(void *self) {
    ++g_netSessionBrowserOnOkOnOkCalls;
    g_netSessionBrowserOnOkOnOkThis = self;
}

void ResetNetSessionBrowserOnOkLog() {
    g_netSessionBrowserOnOkValidateCalls = 0;
    g_netSessionBrowserOnOkValidateThis = nullptr;
    g_netSessionBrowserOnOkSetModemCalls = 0;
    g_netSessionBrowserOnOkSetModemValue = -1;
    g_netSessionBrowserOnOkKillTimerCalls = 0;
    g_netSessionBrowserOnOkKillTimerHwnd = nullptr;
    g_netSessionBrowserOnOkKillTimerId = 0;
    g_netSessionBrowserOnOkSendMessageCalls = 0;
    g_netSessionBrowserOnOkItemDataIndex = 0;
    g_netSessionBrowserOnOkRefreshCalls = 0;
    g_netSessionBrowserOnOkRefreshThis = nullptr;
    g_netSessionBrowserOnOkOnOkCalls = 0;
    g_netSessionBrowserOnOkOnOkThis = nullptr;
}

int WINAPI FakeNetUiWsaStartup(WORD versionRequested, void *wsaData) {
    ++g_netUiWsaStartupCalls;
    g_netUiWsaStartupVersion = versionRequested;
    if (wsaData != nullptr) {
        unsigned char *const bytes = reinterpret_cast<unsigned char *>(wsaData);
        *reinterpret_cast<WORD *>(bytes + 2) = g_netUiWsaStartupHighVersion;
    }
    return g_netUiWsaStartupResult;
}

int WINAPI FakeNetUiWsaCleanup() {
    ++g_netUiWsaCleanupCalls;
    return 0;
}

BOOL WINAPI FakeNetUiMessageBeep(UINT type) {
    ++g_netUiMessageBeepCalls;
    g_netUiMessageBeepType = type;
    return TRUE;
}

HWND WINAPI FakeNetUiGetFocus() {
    ++g_netUiGetFocusCalls;
    return g_netUiGetFocusResult;
}

int WINAPI FakeNetUiMessageBoxA(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type) {
    ++g_netUiMessageBoxCalls;
    g_netUiMessageBoxHwnd = hwnd;
    std::strncpy(g_netUiMessageBoxText, text != nullptr ? text : "",
                 sizeof(g_netUiMessageBoxText));
    g_netUiMessageBoxText[sizeof(g_netUiMessageBoxText) - 1] = '\0';
    std::strncpy(g_netUiMessageBoxCaption, caption != nullptr ? caption : "",
                 sizeof(g_netUiMessageBoxCaption));
    g_netUiMessageBoxCaption[sizeof(g_netUiMessageBoxCaption) - 1] = '\0';
    g_netUiMessageBoxType = type;
    return g_netUiMessageBoxResult;
}

void ResetNetUiWinsockPromptLog() {
    g_netUiWsaStartupCalls = 0;
    g_netUiWsaStartupVersion = 0;
    g_netUiWsaCleanupCalls = 0;
    g_netUiMessageBeepCalls = 0;
    g_netUiMessageBeepType = 0;
    g_netUiGetFocusCalls = 0;
    g_netUiMessageBoxCalls = 0;
    g_netUiMessageBoxHwnd = nullptr;
    g_netUiMessageBoxText[0] = '\0';
    g_netUiMessageBoxCaption[0] = '\0';
    g_netUiMessageBoxType = 0;
}

HRESULT WINAPI FakeDPlayCreateCoInitialize(void *reserved) {
    ++g_dplayCreateCoInitializeCalls;
    g_dplayCreateCoInitializeReserved = reserved;
    return S_OK;
}

HRESULT WINAPI FakeDPlayCreateCoCreateInstance(REFCLSID rclsid, LPUNKNOWN outer,
                                               DWORD clsContext, REFIID riid,
                                               LPVOID *outObject) {
    ++g_dplayCreateCoCreateCalls;
    g_dplayCreateCoCreateArgsOk =
        IsEqualGUID(rclsid, CLSID_DirectPlay) != 0 &&
        outer == nullptr &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, IID_IDirectPlay4A) != 0 &&
        outObject != nullptr;
    g_dplayCreateCoCreateInitialOut = outObject != nullptr ? *outObject : nullptr;
    if (outObject != nullptr && g_dplayCreateCoCreateWriteOut) {
        *outObject = g_dplayCreateCoCreateOut;
    }
    return g_dplayCreateCoCreateResult;
}

int __fastcall FakeDPlayCreateReportError(int hresult, const char *,
                                               int sourceLine) {
    ++g_dplayCreateReportErrorCalls;
    g_dplayCreateReportErrorHresult = hresult;
    g_dplayCreateReportErrorLine = sourceLine;
    return hresult == 0 ? 1 : 0;
}

void FakeDPlayCreateReportOld(int flags, const char *, int sourceLine,
                                           const char *format, ...) {
    ++g_dplayCreateReportOldCalls;
    g_dplayCreateReportOldFlags = flags;
    g_dplayCreateReportOldLine = sourceLine;
    std::strncpy(g_dplayCreateReportOldMessage, format != nullptr ? format : "",
                 sizeof(g_dplayCreateReportOldMessage));
    g_dplayCreateReportOldMessage[sizeof(g_dplayCreateReportOldMessage) - 1] = '\0';
}

void ResetDPlayCreateInterfaceLog() {
    g_dplayCreateCoInitializeCalls = 0;
    g_dplayCreateCoInitializeReserved = nullptr;
    g_dplayCreateCoCreateCalls = 0;
    g_dplayCreateCoCreateArgsOk = false;
    g_dplayCreateCoCreateInitialOut = nullptr;
    g_dplayCreateCoCreateWriteOut = true;
    g_dplayCreateReportErrorCalls = 0;
    g_dplayCreateReportErrorHresult = 0;
    g_dplayCreateReportErrorLine = 0;
    g_dplayCreateReportOldCalls = 0;
    g_dplayCreateReportOldFlags = 0;
    g_dplayCreateReportOldLine = 0;
    g_dplayCreateReportOldMessage[0] = '\0';
}

int __fastcall FakeDPlaySelectClose(zNetwork_DPlay4 *directPlay4) {
    ++g_dplaySelectCloseCalls;
    g_dplaySelectCloseArg = directPlay4;
    return 0;
}

int __fastcall FakeDPlaySelectCreate(zNetwork_DPlay4 **outDirectPlay4) {
    ++g_dplaySelectCreateCalls;
    g_dplaySelectCreateOutPtr = outDirectPlay4;
    if (outDirectPlay4 != nullptr) {
        *outDirectPlay4 = g_dplaySelectCreateOut;
    }
    return g_dplaySelectCreateResult;
}

int __fastcall
FakeDPlaySelectInitialize(zNetworkDPlayServiceProviderInfo *providerInfo) {
    ++g_dplaySelectInitializeCalls;
    g_dplaySelectInitializeProvider = providerInfo;
    return g_dplaySelectInitializeResult;
}

int __fastcall FakeDPlaySelectReportError(int hresult, const char *, int sourceLine) {
    ++g_dplaySelectReportErrorCalls;
    g_dplaySelectReportErrorHresult = hresult;
    g_dplaySelectReportErrorLine = sourceLine;
    return g_dplaySelectReportErrorResult;
}

void ResetDPlaySelectProviderLog() {
    g_dplaySelectCloseCalls = 0;
    g_dplaySelectCloseArg = nullptr;
    g_dplaySelectCreateCalls = 0;
    g_dplaySelectCreateOutPtr = nullptr;
    g_dplaySelectInitializeCalls = 0;
    g_dplaySelectInitializeProvider = nullptr;
    g_dplaySelectReportErrorCalls = 0;
    g_dplaySelectReportErrorHresult = 0;
    g_dplaySelectReportErrorLine = 0;
}

BOOL WINAPI FakeNetSessionBrowserConnectKillTimer(HWND hwnd, UINT_PTR timerId) {
    ++g_netSessionBrowserConnectKillTimerCalls;
    g_netSessionBrowserConnectKillTimerHwnd = hwnd;
    g_netSessionBrowserConnectKillTimerId = timerId;
    return TRUE;
}

UINT_PTR WINAPI FakeNetSessionBrowserConnectSetTimer(HWND hwnd, UINT_PTR timerId,
                                                     UINT elapsedMs, TIMERPROC) {
    ++g_netSessionBrowserConnectSetTimerCalls;
    g_netSessionBrowserConnectSetTimerHwnd = hwnd;
    g_netSessionBrowserConnectSetTimerId = timerId;
    g_netSessionBrowserConnectSetTimerMs = elapsedMs;
    return timerId;
}

LRESULT WINAPI FakeNetSessionBrowserConnectSendMessageA(HWND hwnd, UINT msg,
                                                        WPARAM wParam,
                                                        LPARAM) {
    ++g_netSessionBrowserConnectSendMessageCalls;
    if (msg == CB_GETCURSEL) {
        return g_netSessionBrowserConnectSelectedIndex;
    }
    if (msg == CB_GETITEMDATA) {
        return (LRESULT)g_netSessionBrowserConnectItemData;
    }
    if (msg == CB_SETCURSEL) {
        ++g_netSessionBrowserConnectSetCurSelCalls;
        g_netSessionBrowserConnectSetCurSelWParam = wParam;
        return 0;
    }
    if (msg == LB_RESETCONTENT) {
        ++g_netSessionBrowserConnectListResetCalls;
        return 0;
    }
    return (LRESULT)hwnd;
}

BOOL __fastcall FakeNetSessionBrowserConnectEnableWindow(void *self, void *,
                                                              BOOL enable) {
    const int index = g_netSessionBrowserConnectEnableCalls++;
    if (index < 8) {
        g_netSessionBrowserConnectEnableThis[index] = self;
        g_netSessionBrowserConnectEnableValue[index] = enable;
    }
    return TRUE;
}

void __fastcall FakeNetSessionBrowserConnectSetWindowTextA(void *self, void *,
                                                                LPCSTR text) {
    const int index = g_netSessionBrowserConnectSetTextCalls++;
    if (index < 8) {
        g_netSessionBrowserConnectSetTextThis[index] = self;
        std::strncpy(g_netSessionBrowserConnectSetTextValue[index],
                     text != nullptr ? text : "",
                     sizeof(g_netSessionBrowserConnectSetTextValue[index]));
        g_netSessionBrowserConnectSetTextValue[index]
            [sizeof(g_netSessionBrowserConnectSetTextValue[index]) - 1] = '\0';
    }
}

int __fastcall FakeNetSessionBrowserConnectVerify(const char *caption,
                                                       const char *messageFormat) {
    ++g_netSessionBrowserConnectVerifyCalls;
    std::strncpy(g_netSessionBrowserConnectVerifyCaption,
                 caption != nullptr ? caption : "",
                 sizeof(g_netSessionBrowserConnectVerifyCaption));
    g_netSessionBrowserConnectVerifyCaption
        [sizeof(g_netSessionBrowserConnectVerifyCaption) - 1] = '\0';
    std::strncpy(g_netSessionBrowserConnectVerifyFormat,
                 messageFormat != nullptr ? messageFormat : "",
                 sizeof(g_netSessionBrowserConnectVerifyFormat));
    g_netSessionBrowserConnectVerifyFormat
        [sizeof(g_netSessionBrowserConnectVerifyFormat) - 1] = '\0';
    return g_netSessionBrowserConnectVerifyResult;
}

int __fastcall
FakeNetSessionBrowserConnectSelectProvider(zNetworkDPlayServiceProviderInfo *provider) {
    ++g_netSessionBrowserConnectSelectCalls;
    g_netSessionBrowserConnectSelectProvider = provider;
    return 0;
}

int __fastcall FakeNetSessionBrowserConnectRefresh(void *self) {
    ++g_netSessionBrowserConnectRefreshCalls;
    g_netSessionBrowserConnectRefreshThis = self;
    return g_netSessionBrowserConnectRefreshResult;
}

void ResetNetSessionBrowserConnectLog() {
    g_netSessionBrowserConnectKillTimerCalls = 0;
    g_netSessionBrowserConnectKillTimerHwnd = nullptr;
    g_netSessionBrowserConnectKillTimerId = 0;
    g_netSessionBrowserConnectSendMessageCalls = 0;
    g_netSessionBrowserConnectSetCurSelCalls = 0;
    g_netSessionBrowserConnectSetCurSelWParam = 0;
    g_netSessionBrowserConnectListResetCalls = 0;
    g_netSessionBrowserConnectEnableCalls = 0;
    std::memset(g_netSessionBrowserConnectEnableThis, 0,
                sizeof(g_netSessionBrowserConnectEnableThis));
    std::memset(g_netSessionBrowserConnectEnableValue, 0,
                sizeof(g_netSessionBrowserConnectEnableValue));
    g_netSessionBrowserConnectSetTextCalls = 0;
    std::memset(g_netSessionBrowserConnectSetTextThis, 0,
                sizeof(g_netSessionBrowserConnectSetTextThis));
    std::memset(g_netSessionBrowserConnectSetTextValue, 0,
                sizeof(g_netSessionBrowserConnectSetTextValue));
    g_netSessionBrowserConnectSetTimerCalls = 0;
    g_netSessionBrowserConnectSetTimerHwnd = nullptr;
    g_netSessionBrowserConnectSetTimerId = 0;
    g_netSessionBrowserConnectSetTimerMs = 0;
    g_netSessionBrowserConnectVerifyCalls = 0;
    g_netSessionBrowserConnectVerifyCaption[0] = '\0';
    g_netSessionBrowserConnectVerifyFormat[0] = '\0';
    g_netSessionBrowserConnectSelectCalls = 0;
    g_netSessionBrowserConnectSelectProvider = nullptr;
    g_netSessionBrowserConnectRefreshCalls = 0;
    g_netSessionBrowserConnectRefreshThis = nullptr;
}

BOOL __fastcall FakeNetSessionBrowserOnInitBase(void *self) {
    ++g_netSessionBrowserOnInitBaseCalls;
    g_netSessionBrowserOnInitBaseThis = self;
    return TRUE;
}

char *FakeNetSessionBrowserOnInitGetPlayerName() {
    return g_netSessionBrowserOnInitPlayerName;
}

zNetworkServiceProviderListVec *FakeNetSessionBrowserOnInitRefreshProviders() {
    ++g_netSessionBrowserOnInitRefreshCalls;
    return g_netSessionBrowserOnInitProviderList;
}

LRESULT WINAPI FakeNetSessionBrowserOnInitSendMessageA(HWND, UINT msg, WPARAM wParam,
                                                       LPARAM lParam) {
    ++g_netSessionBrowserOnInitSendMessageCalls;
    if (msg == CB_ADDSTRING) {
        const int index = g_netSessionBrowserOnInitAddStringCalls++;
        if (index < 8) {
            std::strncpy(g_netSessionBrowserOnInitAddStringText[index],
                         lParam != 0 ? reinterpret_cast<const char *>(lParam) : "",
                         sizeof(g_netSessionBrowserOnInitAddStringText[index]));
            g_netSessionBrowserOnInitAddStringText[index]
                [sizeof(g_netSessionBrowserOnInitAddStringText[index]) - 1] = '\0';
        }
        return index;
    }
    if (msg == CB_SETITEMDATA) {
        const int index = g_netSessionBrowserOnInitSetItemDataCalls++;
        if (index < 8) {
            g_netSessionBrowserOnInitSetItemDataIndex[index] = wParam;
            g_netSessionBrowserOnInitSetItemDataValue[index] = lParam;
        }
        return 0;
    }
    if (msg == CB_SETCURSEL) {
        ++g_netSessionBrowserOnInitSetCurSelCalls;
        g_netSessionBrowserOnInitSetCurSelIndex = wParam;
        return 0;
    }
    return 0;
}

void __fastcall FakeNetSessionBrowserOnInitSetWindowTextA(void *, void *,
                                                              LPCSTR text) {
    ++g_netSessionBrowserOnInitSetTextCalls;
    std::strncpy(g_netSessionBrowserOnInitSetTextValue, text != nullptr ? text : "",
                 sizeof(g_netSessionBrowserOnInitSetTextValue));
    g_netSessionBrowserOnInitSetTextValue
        [sizeof(g_netSessionBrowserOnInitSetTextValue) - 1] = '\0';
}

int __fastcall FakeNetSessionBrowserOnInitUpdateData(void *, void *,
                                                         BOOL saveAndValidate) {
    ++g_netSessionBrowserOnInitUpdateDataCalls;
    g_netSessionBrowserOnInitUpdateDataSave = saveAndValidate;
    return 1;
}

void ResetNetSessionBrowserOnInitLog() {
    g_netSessionBrowserOnInitBaseCalls = 0;
    g_netSessionBrowserOnInitBaseThis = nullptr;
    g_netSessionBrowserOnInitRefreshCalls = 0;
    g_netSessionBrowserOnInitSendMessageCalls = 0;
    g_netSessionBrowserOnInitAddStringCalls = 0;
    std::memset(g_netSessionBrowserOnInitAddStringText, 0,
                sizeof(g_netSessionBrowserOnInitAddStringText));
    g_netSessionBrowserOnInitSetItemDataCalls = 0;
    std::memset(g_netSessionBrowserOnInitSetItemDataIndex, 0,
                sizeof(g_netSessionBrowserOnInitSetItemDataIndex));
    std::memset(g_netSessionBrowserOnInitSetItemDataValue, 0,
                sizeof(g_netSessionBrowserOnInitSetItemDataValue));
    g_netSessionBrowserOnInitSetCurSelCalls = 0;
    g_netSessionBrowserOnInitSetCurSelIndex = 0;
    g_netSessionBrowserOnInitSetTextCalls = 0;
    g_netSessionBrowserOnInitSetTextValue[0] = '\0';
    g_netSessionBrowserOnInitUpdateDataCalls = 0;
    g_netSessionBrowserOnInitUpdateDataSave = TRUE;
}

bool RunNetSessionBrowserHelpDocsScenario(NetSessionBrowserDialog &dialog,
                                          UINT_PTR findExecutableResult,
                                          const char *expectedMessageText,
                                          bool expectShellExecute) {
    g_netSessionBrowserHelpFindExecutableResult = findExecutableResult;
    g_netSessionBrowserHelpExpectedMessageText = expectedMessageText;
    g_netSessionBrowserHelpFindExecutableCalls = 0;
    g_netSessionBrowserHelpFindExecutableArgsOk = false;
    g_netSessionBrowserHelpShellExecuteCalls = 0;
    g_netSessionBrowserHelpShellExecuteArgsOk = false;
    g_netSessionBrowserHelpMessageBoxCalls = 0;
    g_netSessionBrowserHelpMessageBoxArgsOk = false;
    g_netSessionBrowserHelpMessageBoxThis = nullptr;

    dialog.OnHelpDocs();

    const bool findOk =
        g_netSessionBrowserHelpFindExecutableCalls == 1 &&
        g_netSessionBrowserHelpFindExecutableArgsOk;
    if (expectShellExecute) {
        return findOk && g_netSessionBrowserHelpShellExecuteCalls == 1 &&
               g_netSessionBrowserHelpShellExecuteArgsOk &&
               g_netSessionBrowserHelpMessageBoxCalls == 0;
    }

    return findOk && g_netSessionBrowserHelpShellExecuteCalls == 0 &&
           g_netSessionBrowserHelpMessageBoxCalls == 1 &&
           g_netSessionBrowserHelpMessageBoxThis == &dialog &&
           g_netSessionBrowserHelpMessageBoxArgsOk;
}

extern "C" int net_session_browser_dialog_get_message_map_smoke(void) {
    unsigned char dialogStorage[sizeof(NetSessionBrowserDialog)] = {0};
    NetSessionBrowserDialog &dialog = *(NetSessionBrowserDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap =
        dialog.NetSessionBrowserDialog::GetMessageMap();
    if (messageMap != &NetSessionBrowserDialog::messageMap ||
        messageMap->pfnGetBaseMap == nullptr ||
        messageMap->pfnGetBaseMap() == nullptr ||
        messageMap->lpEntries != &NetSessionBrowserDialog::messageEntries[0]) {
        return 10;
    }

    const AFX_MSGMAP_ENTRY *const entries = messageMap->lpEntries;
    const bool providerEntryOk =
        entries[0].nMessage == WM_COMMAND &&
        entries[0].nCode == CBN_CLOSEUP &&
        entries[0].nID == 1114 &&
        entries[0].nLastID == 1114 &&
        entries[0].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[0]) ==
            MemberPointerBits(&NetSessionBrowserDialog::ConnectSelectedProvider);
    const bool createEntryOk =
        entries[1].nMessage == WM_COMMAND &&
        entries[1].nCode == BN_CLICKED &&
        entries[1].nID == 1030 &&
        entries[1].nLastID == 1030 &&
        entries[1].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[1]) ==
            MemberPointerBits(&NetSessionBrowserDialog::OnCreateSession);
    const bool timerEntryOk =
        entries[2].nMessage == WM_TIMER &&
        entries[2].nCode == 0 &&
        entries[2].nID == 0 &&
        entries[2].nLastID == 0 &&
        entries[2].nSig == 13 &&
        MsgMapEntryHandlerBits(entries[2]) ==
            MemberPointerBits(&NetSessionBrowserDialog::OnTimer);
    const bool destroyEntryOk =
        entries[3].nMessage == WM_DESTROY &&
        entries[3].nCode == 0 &&
        entries[3].nID == 0 &&
        entries[3].nLastID == 0 &&
        entries[3].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[3]) ==
            MemberPointerBits(&NetSessionBrowserDialog::OnDestroy);
    const bool helpEntryOk =
        entries[4].nMessage == WM_COMMAND &&
        entries[4].nCode == BN_CLICKED &&
        entries[4].nID == 1029 &&
        entries[4].nLastID == 1029 &&
        entries[4].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[4]) ==
            MemberPointerBits(&NetSessionBrowserDialog::OnHelpDocs);
    const bool sentinelOk =
        entries[5].nMessage == 0 &&
        entries[5].nCode == 0 &&
        entries[5].nID == 0 &&
        entries[5].nLastID == 0 &&
        entries[5].nSig == 0 &&
        MsgMapEntryHandlerBits(entries[5]) == 0;

    return providerEntryOk && createEntryOk && timerEntryOk && destroyEntryOk &&
                   helpEntryOk && sentinelOk
               ? 0
               : 11;
}

extern "C" int net_session_config_dialog_get_message_map_smoke(void) {
    unsigned char dialogStorage[sizeof(NetSessionConfigDialog)] = {0};
    NetSessionConfigDialog &dialog = *(NetSessionConfigDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap =
        dialog.NetSessionConfigDialog::GetMessageMap();
    if (messageMap != &NetSessionConfigDialog::messageMap ||
        messageMap->pfnGetBaseMap == nullptr ||
        messageMap->pfnGetBaseMap() == nullptr ||
        messageMap->lpEntries != &NetSessionConfigDialog::messageEntries[0]) {
        return 10;
    }

    const AFX_MSGMAP_ENTRY *const entries = messageMap->lpEntries;
    const bool destroyEntryOk =
        entries[0].nMessage == WM_DESTROY &&
        entries[0].nCode == 0 &&
        entries[0].nID == 0 &&
        entries[0].nLastID == 0 &&
        entries[0].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[0]) ==
            MemberPointerBits(&NetSessionConfigDialog::OnDestroy);
    const bool mapChangedEntryOk =
        entries[1].nMessage == WM_COMMAND &&
        entries[1].nCode == CBN_SELCHANGE &&
        entries[1].nID == 1116 &&
        entries[1].nLastID == 1116 &&
        entries[1].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[1]) ==
            MemberPointerBits(&NetSessionConfigDialog::OnMapChanged);
    const bool sentinelOk =
        entries[2].nMessage == 0 &&
        entries[2].nCode == 0 &&
        entries[2].nID == 0 &&
        entries[2].nLastID == 0 &&
        entries[2].nSig == 0 &&
        MsgMapEntryHandlerBits(entries[2]) == 0;

    return destroyEntryOk && mapChangedEntryOk && sentinelOk ? 0 : 11;
}

extern "C" int net_session_config_dialog_init_map_name_strings_smoke(void) {
    const char *const expectedNames[7] = {
        "RiverWorks",
        "Crater Chaos",
        "Beach Rally",
        "Clone City",
        "Frozen Tundra",
        "Poison Valley",
        "New Clone City",
    };

    NetSessionConfigDialog::InitMapNameStrings();

    int result = 0;
    for (int index = 0; index < 7; ++index) {
        if ((const char *)g_NetSessionConfigDialog_MapNameStrings[index] == nullptr ||
            std::strcmp((const char *)g_NetSessionConfigDialog_MapNameStrings[index],
                        expectedNames[index]) != 0) {
            result = index + 1;
            break;
        }
    }

    for (int index = 6; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
    return result;
}

extern "C" int net_session_config_dialog_register_map_name_cleanup_smoke(void) {
    CodeFunctionPatch patch = {};
    g_netSessionConfigAtexitCalls = 0;
    g_netSessionConfigAtexitCallback = nullptr;
    g_netSessionConfigAtexitResult = 17;

    if (!PatchFunctionJump(reinterpret_cast<void *>(&atexit),
                           reinterpret_cast<void *>(&FakeNetSessionConfigAtexit),
                           patch)) {
        return 1;
    }

    NetSessionConfigDialog::RegisterMapNameCleanup();
    RestoreFunctionPatch(patch);

    if (g_netSessionConfigAtexitCalls != 1) {
        return 2;
    }
    if (g_netSessionConfigAtexitCallback !=
        &NetSessionConfigDialog::CleanupMapNameStringsOnExit) {
        return 3;
    }
    return 0;
}

extern "C" int mission_register_multiplayer_maps_smoke(void) {
    const char *const expectedNames[7] = {
        "RiverWorks",
        "Crater Chaos",
        "Beach Rally",
        "Clone City",
        "Frozen Tundra",
        "Poison Valley",
        "New Clone City",
    };
    CodeFunctionPatch patch = {};
    g_netSessionConfigAtexitCalls = 0;
    g_netSessionConfigAtexitCallback = nullptr;
    g_netSessionConfigAtexitResult = 17;

    if (!PatchFunctionJump(reinterpret_cast<void *>(&atexit),
                           reinterpret_cast<void *>(&FakeNetSessionConfigAtexit),
                           patch)) {
        return 1;
    }

    Mission::RegisterMultiplayerMaps();
    RestoreFunctionPatch(patch);

    int result = 0;
    for (int index = 0; index < 7; ++index) {
        if ((const char *)g_NetSessionConfigDialog_MapNameStrings[index] == nullptr ||
            std::strcmp((const char *)g_NetSessionConfigDialog_MapNameStrings[index],
                        expectedNames[index]) != 0) {
            result = index + 2;
            break;
        }
    }
    if (result == 0 && g_netSessionConfigAtexitCalls != 1) {
        result = 9;
    }
    if (result == 0 &&
        g_netSessionConfigAtexitCallback !=
            &NetSessionConfigDialog::CleanupMapNameStringsOnExit) {
        result = 10;
    }

    for (int index = 6; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
    return result;
}

extern "C" int net_session_config_dialog_cleanup_map_name_strings_on_exit_smoke(void) {
    const WORD kMfc42CStringDtorOrdinal = 800;
    ImportFunctionPatch patch = {};
    g_netSessionBrowserDtorStep = 0;
    std::memset(g_netSessionBrowserDtorOrder, 0, sizeof(g_netSessionBrowserDtorOrder));
    std::memset(g_netSessionBrowserDtorThis, 0, sizeof(g_netSessionBrowserDtorThis));

    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal,
                              reinterpret_cast<void *>(&FakeNetSessionBrowserCStringDtor),
                              patch)) {
        return 1;
    }

    NetSessionConfigDialog::CleanupMapNameStringsOnExit();
    RestoreImportPatch(patch);

    if (g_netSessionBrowserDtorStep != 7) {
        return 2;
    }
    for (int index = 0; index < 7; ++index) {
        if (g_netSessionBrowserDtorOrder[index] != 1 ||
            g_netSessionBrowserDtorThis[index] !=
                &g_NetSessionConfigDialog_MapNameStrings[6 - index]) {
            return 3 + index;
        }
    }
    return 0;
}

int RunNetSessionConfigOnInitScenario(int modemEnabled, LPARAM expectedMaxRange) {
    const char *const expectedNames[7] = {
        "RiverWorks",
        "Crater Chaos",
        "Beach Rally",
        "Clone City",
        "Frozen Tundra",
        "Poison Valley",
        "New Clone City",
    };

    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->Constructor(nullptr);
    dialog->m_defaultExerciseOrdinal = 7;
    dialog->m_mapCombo.m_hWnd = reinterpret_cast<HWND>(0x3101);
    dialog->m_timeLimitSpin.m_hWnd = reinterpret_cast<HWND>(0x3102);
    dialog->m_valueLimitSpin.m_hWnd = reinterpret_cast<HWND>(0x3103);
    dialog->m_maxPlayersSpin.m_hWnd = reinterpret_cast<HWND>(0x3104);

    ResetNetSessionConfigOnInitLog(modemEnabled);
    const BOOL initResult = dialog->NetSessionConfigDialog::OnInitDialog();

    int result = 0;
    if (initResult != TRUE ||
        g_netSessionConfigOnInitBaseCalls != 1 ||
        g_netSessionConfigOnInitBaseThis != dialog) {
        result = 1;
    } else if (std::strcmp((const char *)dialog->m_sessionName, "Exercise 007") != 0) {
        result = 2;
    } else if (g_netSessionConfigSendMessageStep != 18 ||
               g_netSessionConfigAddStringCalls != 7) {
        result = 3;
    } else {
        for (int index = 0; index < 7; ++index) {
            const int addCall = index * 2;
            const int dataCall = addCall + 1;
            if (g_netSessionConfigSendMessageHwndByCall[addCall] !=
                    dialog->m_mapCombo.m_hWnd ||
                g_netSessionConfigSendMessageMsgByCall[addCall] != CB_ADDSTRING ||
                g_netSessionConfigSendMessageWParamByCall[addCall] != 0 ||
                std::strcmp(
                    reinterpret_cast<const char *>(
                        g_netSessionConfigSendMessageLParamByCall[addCall]),
                    expectedNames[index]) != 0 ||
                g_netSessionConfigSendMessageHwndByCall[dataCall] !=
                    dialog->m_mapCombo.m_hWnd ||
                g_netSessionConfigSendMessageMsgByCall[dataCall] != CB_SETITEMDATA ||
                g_netSessionConfigSendMessageWParamByCall[dataCall] !=
                    static_cast<WPARAM>(index) ||
                g_netSessionConfigSendMessageLParamByCall[dataCall] != index) {
                result = 4 + index;
                break;
            }
        }
    }

    if (result == 0) {
        const bool tailMessagesOk =
            g_netSessionConfigSendMessageHwndByCall[14] == dialog->m_mapCombo.m_hWnd &&
            g_netSessionConfigSendMessageMsgByCall[14] == CB_SETCURSEL &&
            g_netSessionConfigSendMessageWParamByCall[14] == 0 &&
            g_netSessionConfigSendMessageLParamByCall[14] == 0 &&
            g_netSessionConfigSendMessageHwndByCall[15] ==
                dialog->m_timeLimitSpin.m_hWnd &&
            g_netSessionConfigSendMessageMsgByCall[15] == 1125 &&
            g_netSessionConfigSendMessageWParamByCall[15] == 0 &&
            g_netSessionConfigSendMessageLParamByCall[15] == MAKELPARAM(360, 0) &&
            g_netSessionConfigSendMessageHwndByCall[16] ==
                dialog->m_valueLimitSpin.m_hWnd &&
            g_netSessionConfigSendMessageMsgByCall[16] == 1125 &&
            g_netSessionConfigSendMessageWParamByCall[16] == 0 &&
            g_netSessionConfigSendMessageLParamByCall[16] == MAKELPARAM(100, 0) &&
            g_netSessionConfigSendMessageHwndByCall[17] ==
                dialog->m_maxPlayersSpin.m_hWnd &&
            g_netSessionConfigSendMessageMsgByCall[17] == 1125 &&
            g_netSessionConfigSendMessageWParamByCall[17] == 0 &&
            g_netSessionConfigSendMessageLParamByCall[17] == expectedMaxRange;
        const bool stateOk =
            dialog->m_valueLimit == 5 &&
            dialog->m_timeLimitMinutes == 10 &&
            dialog->m_maxPlayers == 8 &&
            dialog->m_unusedCheckboxEnabled == 1 &&
            g_netSessionConfigUpdateDataCalls == 1 &&
            g_netSessionConfigUpdateDataSave == FALSE &&
            g_netSessionConfigGetNetworkModemEnabledCalls == 1 &&
            g_netSessionConfigSetDlgItemTextCalls == 1 &&
            g_netSessionConfigSetDlgItemTextThis == dialog &&
            g_netSessionConfigSetDlgItemTextId == 1125 &&
            std::strcmp(g_netSessionConfigSetDlgItemTextValue, "Max players") == 0;
        result = tailMessagesOk && stateOk ? 0 : 20;
    }

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_on_init_dialog_smoke(void) {
    const WORD kMfc42CDialogOnInitDialogOrdinal = 4710;
    const WORD kMfc42CWndSetDlgItemTextOrdinal = 5953;
    ImportFunctionPatch importPatches[3] = {};
    CodeFunctionPatch functionPatches[3] = {};
    const bool installed =
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionConfigSendMessageA),
                          importPatches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnInitDialogOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigOnInitBase),
                             importPatches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigSetDlgItemTextA),
                             importPatches[2]) &&
        PatchFunctionJump(CWndUpdateDataAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionConfigUpdateData),
                          functionPatches[0]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zOpt::GetNetworkModemEnabled),
                          reinterpret_cast<void *>(
                              &FakeNetSessionConfigGetNetworkModemEnabled),
                          functionPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                          reinterpret_cast<void *>(
                              &FakeNetSessionBrowserHelpGetMessageString),
                          functionPatches[2]);

    NetSessionConfigDialog::InitMapNameStrings();
    int result = 0;
    if (!installed) {
        result = 1;
    } else {
        result = RunNetSessionConfigOnInitScenario(0, MAKELPARAM(8, 2));
        if (result == 0) {
            const int modemResult =
                RunNetSessionConfigOnInitScenario(1, MAKELPARAM(2, 2));
            if (modemResult != 0) {
                result = 100 + modemResult;
            }
        }
    }
    NetSessionConfigDialog::CleanupMapNameStringsOnExit();

    for (int index = 2; index >= 0; --index) {
        RestoreFunctionPatch(functionPatches[index]);
    }
    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    return result;
}

extern "C" int net_session_browser_dialog_on_init_dialog_smoke(void) {
    const WORD kMfc42CDialogOnInitDialogOrdinal = 4710;
    ImportFunctionPatch importPatches[2] = {};
    CodeFunctionPatch functionPatches[5] = {};
    const bool installed =
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitSendMessageA),
                          importPatches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnInitDialogOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitBase),
                             importPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zOpt_GetPlayerName),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitGetPlayerName),
                          functionPatches[0]) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetworkDPlay::RefreshAndGetServiceProviderList),
            reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitRefreshProviders),
            functionPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserHelpGetMessageString),
                          functionPatches[2]) &&
        PatchFunctionJump(CWndSetWindowTextAAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitSetWindowTextA),
                          functionPatches[3]) &&
        PatchFunctionJump(CWndUpdateDataAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnInitUpdateData),
                          functionPatches[4]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->Constructor(nullptr);
    dialog->m_providerCombo.m_hWnd = reinterpret_cast<HWND>(0x2001);

    char ipxName[] = "IPX LAN";
    char otherName[] = "Serial";
    char tcpName[] = "TCP/IP Direct";
    char modemName[] = "Modem Dial";
    zNetworkDPlayServiceProviderInfo ipx = {};
    zNetworkDPlayServiceProviderInfo other = {};
    zNetworkDPlayServiceProviderInfo tcp = {};
    zNetworkDPlayServiceProviderInfo modem = {};
    ipx.displayName = ipxName;
    other.displayName = otherName;
    tcp.displayName = tcpName;
    modem.displayName = modemName;
    zNetworkDPlayServiceProviderInfo *providers[] = {
        &ipx, &other, &tcp, &modem,
    };
    zNetworkServiceProviderListVec providerList = {};
    providerList.begin = providers;
    providerList.end = providers + 4;
    providerList.cap = providers + 4;
    g_netSessionBrowserOnInitProviderList = &providerList;

    int result = 0;
    if (!installed) {
        result = 10;
    } else {
        ResetNetSessionBrowserOnInitLog();
        dialog->m_shouldEnterHostSetup = 1;
        dialog->m_sessionCount = 3;
        const BOOL initResult = dialog->NetSessionBrowserDialog::OnInitDialog();

        const bool comboOk =
            g_netSessionBrowserOnInitAddStringCalls == 4 &&
            std::strcmp(g_netSessionBrowserOnInitAddStringText[0], "IPX LAN") == 0 &&
            std::strcmp(g_netSessionBrowserOnInitAddStringText[1], "TCP/IP Direct") == 0 &&
            std::strcmp(g_netSessionBrowserOnInitAddStringText[2], "Modem Dial") == 0 &&
            std::strcmp(g_netSessionBrowserOnInitAddStringText[3], "No providers") == 0 &&
            g_netSessionBrowserOnInitSetItemDataCalls == 4 &&
            g_netSessionBrowserOnInitSetItemDataIndex[0] == 0 &&
            g_netSessionBrowserOnInitSetItemDataValue[0] == (LPARAM)&ipx &&
            g_netSessionBrowserOnInitSetItemDataIndex[1] == 1 &&
            g_netSessionBrowserOnInitSetItemDataValue[1] == (LPARAM)&tcp &&
            g_netSessionBrowserOnInitSetItemDataIndex[2] == 2 &&
            g_netSessionBrowserOnInitSetItemDataValue[2] == (LPARAM)&modem &&
            g_netSessionBrowserOnInitSetItemDataIndex[3] == 3 &&
            g_netSessionBrowserOnInitSetItemDataValue[3] == 0 &&
            g_netSessionBrowserOnInitSetCurSelCalls == 1 &&
            g_netSessionBrowserOnInitSetCurSelIndex == 0;
        const bool stateOk =
            initResult == TRUE &&
            g_netSessionBrowserOnInitBaseCalls == 1 &&
            g_netSessionBrowserOnInitBaseThis == dialog &&
            g_netSessionBrowserOnInitRefreshCalls == 1 &&
            dialog->m_shouldEnterHostSetup == 0 &&
            dialog->m_sessionCount == 0 &&
            std::strcmp((const char *)dialog->m_playerName, "Pilot") == 0 &&
            g_netSessionBrowserOnInitSetTextCalls == 1 &&
            std::strcmp(g_netSessionBrowserOnInitSetTextValue, "Join") == 0 &&
            g_netSessionBrowserOnInitUpdateDataCalls == 1 &&
            g_netSessionBrowserOnInitUpdateDataSave == FALSE;

        result = comboOk && stateOk ? 0 : 11;
    }

    for (int index = 4; index >= 0; --index) {
        RestoreFunctionPatch(functionPatches[index]);
    }
    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    dialog->m_playerName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_constructor_smoke(void) {
    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionBrowserDialog *const returned = dialog->Constructor(nullptr);
    const bool ok =
        returned == dialog &&
        TestObjectVtable(dialog) != 0 &&
        TestMfcWindowConstructed(dialog->m_playerNameEdit) &&
        TestMfcWindowConstructed(dialog->m_okButton) &&
        TestMfcWindowConstructed(dialog->m_createSessionButton) &&
        TestMfcWindowConstructed(dialog->m_sessionList) &&
        TestMfcWindowConstructed(dialog->m_providerCombo) &&
        (const char *)dialog->m_playerName != nullptr &&
        std::strcmp((const char *)dialog->m_playerName, "") == 0 &&
        offsetof(NetSessionBrowserDialog, m_playerNameEdit) == 0x70 &&
        offsetof(NetSessionBrowserDialog, m_playerName) == 0x1b0;

    dialog->m_playerName.~CString();
    ::operator delete(dialog);
    return ok ? 0 : 1;
}

extern "C" int net_session_browser_dialog_scalar_deleting_dtor_smoke(void) {
    CodeFunctionPatch patch = {};
    const bool installed =
        PatchFunctionJump(NetSessionBrowserDestructorAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserDestructor),
                          patch);

    unsigned char stackDialogStorage[sizeof(NetSessionBrowserDialog)] = {0};

    NetSessionBrowserDialog &stackDialog = *(NetSessionBrowserDialog *)stackDialogStorage;
    g_netSessionBrowserScalarDtorCalls = 0;
    g_netSessionBrowserScalarDtorThis[0] = nullptr;
    g_netSessionBrowserScalarDtorThis[1] = nullptr;

    NetSessionBrowserDialog *stackResult = nullptr;
    if (installed) {
        stackResult = stackDialog.ScalarDeletingDestructor(0);
    }

    NetSessionBrowserDialog *const heapDialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(heapDialog, 0, sizeof(*heapDialog));
    NetSessionBrowserDialog *heapResult = nullptr;
    if (installed) {
        heapResult = heapDialog->ScalarDeletingDestructor(1);
    } else {
        ::operator delete(heapDialog);
    }

    RestoreFunctionPatch(patch);
    return installed && stackResult == &stackDialog && heapResult == heapDialog &&
                   g_netSessionBrowserScalarDtorCalls == 2 &&
                   g_netSessionBrowserScalarDtorThis[0] == &stackDialog &&
                   g_netSessionBrowserScalarDtorThis[1] == heapDialog
               ? 0
               : 1;
}

extern "C" int net_session_config_dialog_constructor_smoke(void) {
    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(nullptr);
    const bool ok =
        returned == dialog &&
        TestObjectVtable(dialog) != 0 &&
        TestMfcWindowConstructed(dialog->m_maxPlayersSpin) &&
        TestMfcWindowConstructed(dialog->m_valueLimitSpin) &&
        TestMfcWindowConstructed(dialog->m_timeLimitSpin) &&
        TestMfcWindowConstructed(dialog->m_mapCombo) &&
        (const char *)dialog->m_sessionName != nullptr &&
        std::strcmp((const char *)dialog->m_sessionName, "") == 0 &&
        dialog->m_valueLimit == 0 &&
        dialog->m_timeLimitMinutes == 0 &&
        dialog->m_maxPlayers == 0 &&
        dialog->m_unusedCheckboxEnabled == 0 &&
        offsetof(NetSessionConfigDialog, m_maxPlayersSpin) == 0x68 &&
        offsetof(NetSessionConfigDialog, m_sessionName) == 0x168 &&
        sizeof(NetSessionConfigDialog) == 0x17c;

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return ok ? 0 : 1;
}

extern "C" int net_session_config_dialog_destructor_smoke(void) {
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CSpinButtonCtrlDtorOrdinal = 793;
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch patches[4] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserCStringDtor),
                             patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CComboBoxDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserComboDtor),
                             patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CSpinButtonCtrlDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigSpinDtor),
                             patches[2]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDialogDtor),
                             patches[3]);

    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(nullptr);
    dialog->m_sessionName = "session";
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else if (TestObjectVtable(dialog) == 0) {
        result = 4;
    } else if (std::strcmp((const char *)dialog->m_sessionName, "session") != 0) {
        result = 5;
    } else {
        g_netSessionBrowserDtorStep = 0;
        for (int index = 0; index < 7; ++index) {
            g_netSessionBrowserDtorOrder[index] = 0;
            g_netSessionBrowserDtorThis[index] = nullptr;
        }

        dialog->Destructor();

        if (g_netSessionBrowserDtorStep != 6) {
            result = 6;
        } else if (g_netSessionBrowserDtorOrder[0] != 1 ||
                   g_netSessionBrowserDtorOrder[1] != 2 ||
                   g_netSessionBrowserDtorOrder[2] != 3 ||
                   g_netSessionBrowserDtorOrder[3] != 3 ||
                   g_netSessionBrowserDtorOrder[4] != 3 ||
                   g_netSessionBrowserDtorOrder[5] != 7) {
            result = 7;
        } else if (g_netSessionBrowserDtorThis[0] != &dialog->m_sessionName ||
                   g_netSessionBrowserDtorThis[1] != &dialog->m_mapCombo ||
                   g_netSessionBrowserDtorThis[2] != &dialog->m_timeLimitSpin ||
                   g_netSessionBrowserDtorThis[3] != &dialog->m_valueLimitSpin ||
                   g_netSessionBrowserDtorThis[4] != &dialog->m_maxPlayersSpin ||
                   g_netSessionBrowserDtorThis[5] != dialog) {
            result = 8;
        }
    }

    for (int index = 3; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_do_data_exchange_smoke(void) {
    const WORD kMfc42DDVMaxCharsOrdinal = 2289;
    const WORD kMfc42DDVMinMaxUIntOrdinal = 2297;
    const WORD kMfc42DDXCheckOrdinal = 2301;
    const WORD kMfc42DDXControlOrdinal = 2302;
    const WORD kMfc42DDXTextUIntOrdinal = 2363;
    const WORD kMfc42DDXTextCStringOrdinal = 2370;
    ImportFunctionPatch patches[6] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXControlOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDXControl),
                             patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextCStringOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDXText),
                             patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDVMaxCharsOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDVMaxChars),
                             patches[2]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextUIntOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigDDXTextUInt),
                             patches[3]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDVMinMaxUIntOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigDDVMinMaxUInt),
                             patches[4]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXCheckOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigDDXCheck),
                             patches[5]);

    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(nullptr);
    dialog->m_valueLimit = 123;
    dialog->m_timeLimitMinutes = 456;
    dialog->m_maxPlayers = 6;
    unsigned char dataExchangeStorage[16] = {};
    CDataExchange *const dataExchange =
        reinterpret_cast<CDataExchange *>(dataExchangeStorage);
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        g_netSessionBrowserDdxStep = 0;
        for (int index = 0; index < 13; ++index) {
            g_netSessionBrowserDdxKind[index] = 0;
            g_netSessionBrowserDdxIdOrLimit[index] = 0;
            g_netSessionBrowserDdxContext[index] = nullptr;
            g_netSessionBrowserDdxValue[index] = nullptr;
            g_netSessionBrowserDdxMin[index] = 0;
            g_netSessionBrowserDdxMax[index] = 0;
        }

        dialog->NetSessionConfigDialog::DoDataExchange(dataExchange);

        if (g_netSessionBrowserDdxStep != 13) {
            result = 4;
        } else if (g_netSessionBrowserDdxKind[0] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[0] != 1072 ||
                   g_netSessionBrowserDdxValue[0] != &dialog->m_maxPlayersSpin ||
                   g_netSessionBrowserDdxKind[1] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[1] != 1121 ||
                   g_netSessionBrowserDdxValue[1] != &dialog->m_valueLimitSpin ||
                   g_netSessionBrowserDdxKind[2] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[2] != 1120 ||
                   g_netSessionBrowserDdxValue[2] != &dialog->m_timeLimitSpin ||
                   g_netSessionBrowserDdxKind[3] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[3] != 1116 ||
                   g_netSessionBrowserDdxValue[3] != &dialog->m_mapCombo ||
                   g_netSessionBrowserDdxKind[4] != 2 ||
                   g_netSessionBrowserDdxIdOrLimit[4] != 1115 ||
                   g_netSessionBrowserDdxValue[4] != &dialog->m_sessionName ||
                   g_netSessionBrowserDdxKind[5] != 3 ||
                   g_netSessionBrowserDdxIdOrLimit[5] != 80 ||
                   g_netSessionBrowserDdxValue[5] != &dialog->m_sessionName ||
                   g_netSessionBrowserDdxKind[6] != 4 ||
                   g_netSessionBrowserDdxIdOrLimit[6] != 1117 ||
                   g_netSessionBrowserDdxValue[6] != &dialog->m_valueLimit ||
                   g_netSessionBrowserDdxKind[7] != 5 ||
                   g_netSessionBrowserDdxIdOrLimit[7] != 123 ||
                   g_netSessionBrowserDdxMin[7] != 0 ||
                   g_netSessionBrowserDdxMax[7] != 10000 ||
                   g_netSessionBrowserDdxKind[8] != 4 ||
                   g_netSessionBrowserDdxIdOrLimit[8] != 1118 ||
                   g_netSessionBrowserDdxValue[8] != &dialog->m_timeLimitMinutes ||
                   g_netSessionBrowserDdxKind[9] != 5 ||
                   g_netSessionBrowserDdxIdOrLimit[9] != 456 ||
                   g_netSessionBrowserDdxMin[9] != 0 ||
                   g_netSessionBrowserDdxMax[9] != 10000 ||
                   g_netSessionBrowserDdxKind[10] != 4 ||
                   g_netSessionBrowserDdxIdOrLimit[10] != 1119 ||
                   g_netSessionBrowserDdxValue[10] != &dialog->m_maxPlayers ||
                   g_netSessionBrowserDdxKind[11] != 5 ||
                   g_netSessionBrowserDdxIdOrLimit[11] != 6 ||
                   g_netSessionBrowserDdxMin[11] != 2 ||
                   g_netSessionBrowserDdxMax[11] != 8 ||
                   g_netSessionBrowserDdxKind[12] != 6 ||
                   g_netSessionBrowserDdxIdOrLimit[12] != 1122 ||
                   g_netSessionBrowserDdxValue[12] !=
                       &dialog->m_unusedCheckboxEnabled) {
            result = 5;
        } else {
            for (int index = 0; index < 13; ++index) {
                if (g_netSessionBrowserDdxContext[index] != dataExchange) {
                    result = 6;
                    break;
                }
            }
        }
    }

    for (int index = 5; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_on_destroy_smoke(void) {
    const WORD kMfc42CWndOnDestroyOrdinal = 6453;
    ImportFunctionPatch importPatches[2] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndOnDestroyOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserCWndOnDestroy),
                             importPatches[0]) &&
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionConfigSendMessageA),
                          importPatches[1]);

    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(nullptr);
    dialog->m_mapCombo.m_hWnd = reinterpret_cast<HWND>(0x1234);
    g_netSessionBrowserOnDestroyStep = 0;
    g_netSessionBrowserOnDestroyThis = nullptr;
    g_netSessionConfigSendMessageStep = 0;
    g_netSessionConfigSendMessageHwnd = nullptr;
    g_netSessionConfigSelectedComboIndex = 4;
    g_netSessionConfigSelectedItemData = 9;
    g_netSessionConfigItemDataWParam = 0;

    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        dialog->OnDestroy();
        if (g_netSessionBrowserOnDestroyStep != 1 ||
            g_netSessionBrowserOnDestroyThis != dialog) {
            result = 4;
        } else if (g_netSessionConfigSendMessageStep != 2 ||
                   g_netSessionConfigSendMessageHwnd != dialog->m_mapCombo.m_hWnd ||
                   g_netSessionConfigItemDataWParam != 4 ||
                   dialog->m_selectedMapIndex != 9) {
            result = 5;
        }
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

bool RunNetSessionConfigMapChangedScenario(NetSessionConfigDialog *dialog,
                                           LRESULT selectedMapIndex,
                                           const char *expectedText) {
    g_netSessionConfigSendMessageStep = 0;
    g_netSessionConfigSendMessageHwnd = nullptr;
    g_netSessionConfigSelectedComboIndex = 3;
    g_netSessionConfigSelectedItemData = selectedMapIndex;
    g_netSessionConfigItemDataWParam = 0;
    g_netSessionConfigSetDlgItemTextCalls = 0;
    g_netSessionConfigSetDlgItemTextThis = nullptr;
    g_netSessionConfigSetDlgItemTextId = 0;
    g_netSessionConfigSetDlgItemTextValue = nullptr;

    dialog->OnMapChanged();

    return g_netSessionConfigSendMessageStep == 2 &&
           g_netSessionConfigSendMessageHwnd == dialog->m_mapCombo.m_hWnd &&
           g_netSessionConfigItemDataWParam == 3 &&
           dialog->m_selectedMapIndex == selectedMapIndex &&
           g_netSessionConfigSetDlgItemTextCalls == 1 &&
           g_netSessionConfigSetDlgItemTextThis == dialog &&
           g_netSessionConfigSetDlgItemTextId == 1125 &&
           g_netSessionConfigSetDlgItemTextValue == expectedText;
}

extern "C" int net_session_config_dialog_on_map_changed_smoke(void) {
    const WORD kMfc42CWndSetDlgItemTextOrdinal = 5953;
    ImportFunctionPatch importPatches[2] = {};
    CodeFunctionPatch functionPatch = {};
    bool installed =
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionConfigSendMessageA),
                          importPatches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionConfigSetDlgItemTextA),
                             importPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserHelpGetMessageString),
                          functionPatch);

    NetSessionConfigDialog *const dialog =
        reinterpret_cast<NetSessionConfigDialog *>(
            ::operator new(sizeof(NetSessionConfigDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(nullptr);
    dialog->m_mapCombo.m_hWnd = reinterpret_cast<HWND>(0x5678);
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else if (!RunNetSessionConfigMapChangedScenario(
                   dialog, 2, FakeNetSessionBrowserHelpGetMessageString(12352))) {
        result = 4;
    } else if (!RunNetSessionConfigMapChangedScenario(
                   dialog, 5, FakeNetSessionBrowserHelpGetMessageString(12353))) {
        result = 5;
    }

    RestoreFunctionPatch(functionPatch);
    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_destructor_smoke(void) {
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CListBoxDtorOrdinal = 692;
    const WORD kMfc42CButtonDtorOrdinal = 609;
    const WORD kMfc42CEditDtorOrdinal = 656;
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch patches[6] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserCStringDtor),
                             patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CComboBoxDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserComboDtor),
                             patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CListBoxDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserListDtor),
                             patches[2]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CButtonDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserCreateButtonDtor),
                             patches[3]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CEditDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserEditDtor),
                             patches[4]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDialogDtor),
                             patches[5]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionBrowserDialog *const returned = dialog->Constructor(nullptr);
    dialog->m_playerName = "pilot";
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else if (TestObjectVtable(dialog) == 0) {
        result = 4;
    } else if (std::strcmp((const char *)dialog->m_playerName, "pilot") != 0) {
        result = 5;
    } else {
        g_netSessionBrowserDtorStep = 0;
        for (int index = 0; index < 7; ++index) {
            g_netSessionBrowserDtorOrder[index] = 0;
            g_netSessionBrowserDtorThis[index] = nullptr;
        }

        dialog->Destructor();

        if (g_netSessionBrowserDtorStep != 7) {
            result = 6;
        } else if (g_netSessionBrowserDtorOrder[0] != 1 ||
                   g_netSessionBrowserDtorOrder[1] != 2 ||
                   g_netSessionBrowserDtorOrder[2] != 3 ||
                   g_netSessionBrowserDtorOrder[3] != 4 ||
                   g_netSessionBrowserDtorOrder[4] != 4 ||
                   g_netSessionBrowserDtorOrder[5] != 6 ||
                   g_netSessionBrowserDtorOrder[6] != 7) {
            result = 7;
        } else if (g_netSessionBrowserDtorThis[0] != &dialog->m_playerName ||
                   g_netSessionBrowserDtorThis[1] != &dialog->m_providerCombo ||
                   g_netSessionBrowserDtorThis[2] != &dialog->m_sessionList ||
                   g_netSessionBrowserDtorThis[3] != &dialog->m_createSessionButton ||
                   g_netSessionBrowserDtorThis[4] != &dialog->m_okButton ||
                   g_netSessionBrowserDtorThis[5] != &dialog->m_playerNameEdit ||
                   g_netSessionBrowserDtorThis[6] != dialog) {
            result = 8;
        }
    }

    for (int index = 5; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }

    dialog->m_playerName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_on_destroy_smoke(void) {
    const WORD kMfc42CWndOnDestroyOrdinal = 6453;
    ImportFunctionPatch patches[2] = {};
    const bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndOnDestroyOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserCWndOnDestroy),
                             patches[0]) &&
        PatchImportByName("USER32.dll", "KillTimer",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserKillTimer),
                          patches[1]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));
    dialog->m_hWnd = reinterpret_cast<HWND>(0x12345678);

    g_netSessionBrowserOnDestroyStep = 0;
    g_netSessionBrowserOnDestroyThis = nullptr;
    g_netSessionBrowserKillTimerHwnd = nullptr;
    g_netSessionBrowserKillTimerId = 0;

    int result = 0;
    if (!installed) {
        result = 2;
    } else {
        dialog->OnDestroy();
        if (g_netSessionBrowserOnDestroyStep != 2) {
            result = 3;
        } else if (g_netSessionBrowserOnDestroyThis != dialog) {
            result = 4;
        } else if (g_netSessionBrowserKillTimerHwnd != dialog->m_hWnd ||
                   g_netSessionBrowserKillTimerId != 2) {
            result = 5;
        }
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }

    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_on_help_docs_smoke(void) {
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    CodeFunctionPatch zlocPatch = {};
    ImportFunctionPatch patches[3] = {};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserHelpGetMessageString),
                           zlocPatch)) {
        return 10;
    }

    const bool installed =
        PatchImportByName("SHELL32.dll", "FindExecutableA",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserHelpFindExecutableA),
                          patches[0]) &&
        PatchImportByName("SHELL32.dll", "ShellExecuteA",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserHelpShellExecuteA),
                          patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndMessageBoxAOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserHelpMessageBoxA),
                             patches[2]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    const HWND oldMainHwnd = g_RecoilApp_hWndMain;
    g_RecoilApp_hWndMain = reinterpret_cast<HWND>(0x12345678);

    int result = 0;
    if (!installed) {
        result = 11;
    } else {
        const bool successOk =
            RunNetSessionBrowserHelpDocsScenario(*dialog, 33, nullptr, true);
        const bool defaultFailureFallsThroughOk =
            RunNetSessionBrowserHelpDocsScenario(*dialog, 1, nullptr, true);
        const bool noAssociationOk =
            RunNetSessionBrowserHelpDocsScenario(
                *dialog, 0, FakeNetSessionBrowserHelpGetMessageString(32), false);
        const bool fileNotFoundOk =
            RunNetSessionBrowserHelpDocsScenario(
                *dialog, 2, FakeNetSessionBrowserHelpGetMessageString(34), false);
        const bool incompleteAssociationOk =
            RunNetSessionBrowserHelpDocsScenario(
                *dialog, 11, FakeNetSessionBrowserHelpGetMessageString(36), false);
        const bool noDdeAssociationOk =
            RunNetSessionBrowserHelpDocsScenario(
                *dialog, 31, FakeNetSessionBrowserHelpGetMessageString(33), false);

        result = successOk && defaultFailureFallsThroughOk && noAssociationOk &&
                         fileNotFoundOk && incompleteAssociationOk &&
                         noDdeAssociationOk
                     ? 0
                     : 12;
    }

    g_RecoilApp_hWndMain = oldMainHwnd;
    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    RestoreFunctionPatch(zlocPatch);

    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_refresh_session_list_smoke(void) {
    CodeFunctionPatch enumSessionsPatch = {};
    CodeFunctionPatch formatMessagePatch = {};
    ImportFunctionPatch sendMessagePatch = {};

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay::EnumSessions),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserRefreshEnumSessions),
                           enumSessionsPatch)) {
        return 10;
    }

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::FormatMessage),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserRefreshFormatMessage),
                           formatMessagePatch)) {
        RestoreFunctionPatch(enumSessionsPatch);
        return 11;
    }

    if (!PatchImportByName("USER32.dll", "SendMessageA",
                           reinterpret_cast<void *>(&FakeNetSessionBrowserRefreshSendMessageA),
                           sendMessagePatch)) {
        RestoreFunctionPatch(formatMessagePatch);
        RestoreFunctionPatch(enumSessionsPatch);
        return 12;
    }

    zNetworkDPlaySessionDescCache session0 = {};
    zNetworkDPlaySessionDescCache session1 = {};
    session0.desc.dwMaxPlayers = 8;
    session0.desc.dwCurrentPlayers = 2;
    session0.desc.lpszSessionNameA = const_cast<char *>("alpha");
    session1.desc.dwMaxPlayers = 12;
    session1.desc.dwCurrentPlayers = 5;
    session1.desc.lpszSessionNameA = const_cast<char *>("bravo");

    zArchiveListNode node0 = {};
    zArchiveListNode node1 = {};
    node0.payload = &session0;
    node0.next = &node1;
    node0.prev = &node1;
    node1.payload = &session1;
    node1.next = &node0;
    node1.prev = &node0;

    zArchiveList sessionList = {};
    sessionList.count = 2;
    sessionList.head = &node0;
    zArchiveList *const oldEnumeratedList = g_zNetwork_EnumeratedSessionList;
    g_zNetwork_EnumeratedSessionList = &sessionList;

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->m_sessionList.m_hWnd = reinterpret_cast<HWND>(0x5150);

    g_netSessionBrowserRefreshEnumSessionsCalls = 0;
    g_netSessionBrowserRefreshEnumSessionsResult = 2;
    g_netSessionBrowserRefreshSendMessageStep = 0;
    g_netSessionBrowserRefreshAddedCount = 0;
    g_netSessionBrowserRefreshSelectedIndex = -1;
    g_netSessionBrowserRefreshResetSeen = false;
    g_netSessionBrowserRefreshMessagesOk = true;
    g_netSessionBrowserRefreshFormatCalls = 0;
    for (int index = 0; index < 4; ++index) {
        g_netSessionBrowserRefreshRows[index][0] = '\0';
        g_netSessionBrowserRefreshItemData[index] = -1;
        g_netSessionBrowserRefreshFormatMessageIds[index] = 0;
        g_netSessionBrowserRefreshFormatMaxChars[index] = 0;
    }

    int result = 0;
    if (dialog->RefreshSessionList() != 2 || dialog->m_sessionCount != 2) {
        result = 13;
    } else if (g_netSessionBrowserRefreshEnumSessionsCalls != 1 ||
               !g_netSessionBrowserRefreshResetSeen ||
               !g_netSessionBrowserRefreshMessagesOk) {
        result = 14;
    } else if (g_netSessionBrowserRefreshFormatCalls != 2 ||
               g_netSessionBrowserRefreshFormatMessageIds[0] != 0x112 ||
               g_netSessionBrowserRefreshFormatMessageIds[1] != 0x112 ||
               g_netSessionBrowserRefreshFormatMaxChars[0] != 120 ||
               g_netSessionBrowserRefreshFormatMaxChars[1] != 120) {
        result = 15;
    } else if (std::strcmp(g_netSessionBrowserRefreshRows[0], "alpha:2/8") != 0 ||
               std::strcmp(g_netSessionBrowserRefreshRows[1], "bravo:5/12") != 0 ||
               g_netSessionBrowserRefreshItemData[0] != 0 ||
               g_netSessionBrowserRefreshItemData[1] != 1) {
        result = 16;
    } else if (g_netSessionBrowserRefreshSelectedIndex != 0) {
        result = 17;
    }

    ::operator delete(dialog);
    g_zNetwork_EnumeratedSessionList = oldEnumeratedList;
    RestoreImportPatch(sendMessagePatch);
    RestoreFunctionPatch(formatMessagePatch);
    RestoreFunctionPatch(enumSessionsPatch);
    return result;
}

extern "C" int net_session_browser_dialog_on_timer_smoke(void) {
    const WORD kMfc42CWndDefaultOrdinal = 2379;
    CodeFunctionPatch refreshPatch = {};
    ImportFunctionPatch defaultPatch = {};
    if (!PatchFunctionJump(
            NetSessionBrowserRefreshSessionListAddress(),
            reinterpret_cast<void *>(&FakeNetSessionBrowserRefreshSessionList),
            refreshPatch)) {
        return 10;
    }

    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndDefaultOrdinal,
                              reinterpret_cast<void *>(&FakeNetSessionBrowserCWndDefault),
                              defaultPatch)) {
        RestoreFunctionPatch(refreshPatch);
        return 12;
    }

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));

    g_netSessionBrowserOnTimerRefreshCalls = 0;
    g_netSessionBrowserOnTimerRefreshThis = nullptr;
    g_netSessionBrowserOnTimerDefaultCalls = 0;
    g_netSessionBrowserOnTimerDefaultThis = nullptr;

    dialog->OnTimer(2);

    const bool ok = g_netSessionBrowserOnTimerRefreshCalls == 1 &&
                    g_netSessionBrowserOnTimerRefreshThis == dialog &&
                    g_netSessionBrowserOnTimerDefaultCalls == 1 &&
                    g_netSessionBrowserOnTimerDefaultThis == dialog;

    ::operator delete(dialog);
    RestoreImportPatch(defaultPatch);
    RestoreFunctionPatch(refreshPatch);
    return ok ? 0 : 11;
}

extern "C" int net_session_browser_dialog_validate_player_name_smoke(void) {
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    const WORD kMfc42CWndSetFocusOrdinal = 5981;
    CodeFunctionPatch updateDataPatch = {};
    CodeFunctionPatch zlocPatch = {};
    ImportFunctionPatch patches[2] = {};

    if (!PatchFunctionJump(CWndUpdateDataAddress(),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserValidateUpdateData),
                           updateDataPatch)) {
        return 10;
    }

    if (!PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserHelpGetMessageString),
                           zlocPatch)) {
        RestoreFunctionPatch(updateDataPatch);
        return 11;
    }

    const bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndMessageBoxAOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserValidateMessageBoxA),
                             patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetFocusOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserValidateSetFocus),
                             patches[1]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));
    dialog->Constructor(nullptr);

    int result = 0;
    if (!installed) {
        result = 12;
    } else {
        dialog->m_playerName = "  pilot  ";
        ResetNetSessionBrowserValidateLog();
        const int validResult = dialog->ValidatePlayerName();
        const bool validOk =
            validResult == TRUE &&
            std::strcmp((const char *)dialog->m_playerName, "pilot") == 0 &&
            g_netSessionBrowserValidateUpdateDataCalls == 2 &&
            g_netSessionBrowserValidateUpdateDataSaveValues[0] == TRUE &&
            g_netSessionBrowserValidateUpdateDataSaveValues[1] == FALSE &&
            g_netSessionBrowserValidateUpdateDataThis[0] == dialog &&
            g_netSessionBrowserValidateUpdateDataThis[1] == dialog &&
            g_netSessionBrowserValidateMessageBoxCalls == 0 &&
            g_netSessionBrowserValidateSetFocusCalls == 0;

        dialog->m_playerName = "   ";
        ResetNetSessionBrowserValidateLog();
        const int invalidResult = dialog->ValidatePlayerName();
        const bool invalidOk =
            invalidResult == FALSE &&
            dialog->m_playerName.IsEmpty() &&
            g_netSessionBrowserValidateUpdateDataCalls == 2 &&
            g_netSessionBrowserValidateUpdateDataSaveValues[0] == TRUE &&
            g_netSessionBrowserValidateUpdateDataSaveValues[1] == FALSE &&
            g_netSessionBrowserValidateUpdateDataThis[0] == dialog &&
            g_netSessionBrowserValidateUpdateDataThis[1] == dialog &&
            g_netSessionBrowserValidateMessageBoxCalls == 1 &&
            g_netSessionBrowserValidateMessageBoxThis == dialog &&
            g_netSessionBrowserValidateMessageBoxArgsOk &&
            g_netSessionBrowserValidateSetFocusCalls == 1 &&
            g_netSessionBrowserValidateSetFocusThis == &dialog->m_playerNameEdit;

        result = validOk && invalidOk ? 0 : 13;
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    RestoreFunctionPatch(zlocPatch);
    RestoreFunctionPatch(updateDataPatch);
    dialog->m_playerName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_on_ok_smoke(void) {
    const WORD kMfc42CDialogOnOKOrdinal = 4853;
    CodeFunctionPatch validatePatch = {};
    CodeFunctionPatch setModemPatch = {};
    CodeFunctionPatch refreshPatch = {};
    ImportFunctionPatch patches[3] = {};

    if (!PatchFunctionJump(NetSessionBrowserValidatePlayerNameAddress(),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkValidate),
                           validatePatch)) {
        return 10;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zOpt::SetNetworkModemEnabled),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkSetModemEnabled),
                           setModemPatch)) {
        RestoreFunctionPatch(validatePatch);
        return 11;
    }
    if (!PatchFunctionJump(NetSessionBrowserRefreshSessionListAddress(),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkRefreshSessionList),
                           refreshPatch)) {
        RestoreFunctionPatch(setModemPatch);
        RestoreFunctionPatch(validatePatch);
        return 12;
    }

    const bool installed =
        PatchImportByName("USER32.dll", "KillTimer",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkKillTimer),
                          patches[0]) &&
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkSendMessageA),
                          patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnOKOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserOnOkOnOK),
                             patches[2]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->m_hWnd = reinterpret_cast<HWND>(0x1001);
    dialog->m_sessionList.m_hWnd = reinterpret_cast<HWND>(0x1002);

    int result = 0;
    if (!installed) {
        result = 13;
    } else {
        g_netSessionBrowserOnOkValidateResult = 0;
        dialog->m_selectedProviderIsModem = 0;
        dialog->m_selectedSessionIndex = 99;
        ResetNetSessionBrowserOnOkLog();
        dialog->NetSessionBrowserDialog::OnOK();
        const bool invalidOk =
            g_netSessionBrowserOnOkValidateCalls == 1 &&
            g_netSessionBrowserOnOkValidateThis == dialog &&
            g_netSessionBrowserOnOkSetModemCalls == 0 &&
            g_netSessionBrowserOnOkKillTimerCalls == 0 &&
            g_netSessionBrowserOnOkOnOkCalls == 0 &&
            dialog->m_selectedSessionIndex == 99;

        g_netSessionBrowserOnOkValidateResult = 1;
        g_netSessionBrowserOnOkListCurSel = LB_ERR;
        dialog->m_selectedProviderIsModem = 0;
        dialog->m_selectedSessionIndex = 99;
        ResetNetSessionBrowserOnOkLog();
        dialog->NetSessionBrowserDialog::OnOK();
        const bool noSelectionOk =
            g_netSessionBrowserOnOkSetModemCalls == 1 &&
            g_netSessionBrowserOnOkSetModemValue == 0 &&
            g_netSessionBrowserOnOkKillTimerCalls == 1 &&
            g_netSessionBrowserOnOkKillTimerHwnd == dialog->m_hWnd &&
            g_netSessionBrowserOnOkKillTimerId == 2 &&
            g_netSessionBrowserOnOkSendMessageCalls == 1 &&
            g_netSessionBrowserOnOkOnOkCalls == 0 &&
            dialog->m_selectedSessionIndex == 99;

        g_netSessionBrowserOnOkValidateResult = 1;
        g_netSessionBrowserOnOkListCurSel = 2;
        g_netSessionBrowserOnOkListItemData = 7;
        dialog->m_selectedProviderIsModem = 0;
        dialog->m_selectedSessionIndex = 99;
        ResetNetSessionBrowserOnOkLog();
        dialog->NetSessionBrowserDialog::OnOK();
        const bool selectedOk =
            g_netSessionBrowserOnOkSetModemCalls == 1 &&
            g_netSessionBrowserOnOkSetModemValue == 0 &&
            g_netSessionBrowserOnOkKillTimerCalls == 1 &&
            g_netSessionBrowserOnOkSendMessageCalls == 2 &&
            g_netSessionBrowserOnOkItemDataIndex == 2 &&
            dialog->m_selectedSessionIndex == 7 &&
            g_netSessionBrowserOnOkOnOkCalls == 1 &&
            g_netSessionBrowserOnOkOnOkThis == dialog;

        g_netSessionBrowserOnOkValidateResult = 1;
        g_netSessionBrowserOnOkRefreshResult = -1;
        dialog->m_selectedProviderIsModem = 1;
        dialog->m_selectedSessionIndex = 99;
        ResetNetSessionBrowserOnOkLog();
        dialog->NetSessionBrowserDialog::OnOK();
        const bool modemRefreshFailOk =
            g_netSessionBrowserOnOkSetModemCalls == 1 &&
            g_netSessionBrowserOnOkSetModemValue == 1 &&
            g_netSessionBrowserOnOkRefreshCalls == 1 &&
            g_netSessionBrowserOnOkRefreshThis == dialog &&
            g_netSessionBrowserOnOkKillTimerCalls == 0 &&
            g_netSessionBrowserOnOkSendMessageCalls == 0 &&
            g_netSessionBrowserOnOkOnOkCalls == 0 &&
            dialog->m_selectedSessionIndex == 99;

        g_netSessionBrowserOnOkValidateResult = 1;
        g_netSessionBrowserOnOkRefreshResult = 0;
        dialog->m_selectedProviderIsModem = 1;
        dialog->m_selectedSessionIndex = 99;
        ResetNetSessionBrowserOnOkLog();
        dialog->NetSessionBrowserDialog::OnOK();
        const bool modemRefreshOk =
            g_netSessionBrowserOnOkSetModemCalls == 1 &&
            g_netSessionBrowserOnOkSetModemValue == 1 &&
            g_netSessionBrowserOnOkRefreshCalls == 1 &&
            dialog->m_selectedSessionIndex == 0 &&
            g_netSessionBrowserOnOkOnOkCalls == 1 &&
            g_netSessionBrowserOnOkOnOkThis == dialog;

        result = invalidOk && noSelectionOk && selectedOk &&
                         modemRefreshFailOk && modemRefreshOk
                     ? 0
                     : 14;
    }

    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    RestoreFunctionPatch(refreshPatch);
    RestoreFunctionPatch(setModemPatch);
    RestoreFunctionPatch(validatePatch);
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_on_create_session_smoke(void) {
    const WORD kMfc42CDialogOnOKOrdinal = 4853;
    CodeFunctionPatch validatePatch = {};
    CodeFunctionPatch createSessionPatch = {};
    CodeFunctionPatch setNetworkPatch = {};
    CodeFunctionPatch setModemPatch = {};
    CodeFunctionPatch getPlayerNamePatch = {};
    CodeFunctionPatch createLocalPlayerPatch = {};
    ImportFunctionPatch patches[2] = {};

    if (!PatchFunctionJump(NetSessionBrowserValidatePlayerNameAddress(),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateValidate),
                           validatePatch)) {
        return 10;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay::CreateSessionFromStatusFields),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateCreateSession),
                           createSessionPatch)) {
        RestoreFunctionPatch(validatePatch);
        return 11;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zOpt::SetNetworkEnabled),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateSetNetworkEnabled),
                           setNetworkPatch)) {
        RestoreFunctionPatch(createSessionPatch);
        RestoreFunctionPatch(validatePatch);
        return 12;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zOpt::SetNetworkModemEnabled),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateSetModemEnabled),
                           setModemPatch)) {
        RestoreFunctionPatch(setNetworkPatch);
        RestoreFunctionPatch(createSessionPatch);
        RestoreFunctionPatch(validatePatch);
        return 13;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zOpt_GetPlayerName),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateGetPlayerName),
                           getPlayerNamePatch)) {
        RestoreFunctionPatch(setModemPatch);
        RestoreFunctionPatch(setNetworkPatch);
        RestoreFunctionPatch(createSessionPatch);
        RestoreFunctionPatch(validatePatch);
        return 14;
    }
    if (!PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay::CreateLocalPlayerRecordAndRegister),
                           reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateCreateLocalPlayer),
                           createLocalPlayerPatch)) {
        RestoreFunctionPatch(getPlayerNamePatch);
        RestoreFunctionPatch(setModemPatch);
        RestoreFunctionPatch(setNetworkPatch);
        RestoreFunctionPatch(createSessionPatch);
        RestoreFunctionPatch(validatePatch);
        return 15;
    }

    const bool installed =
        PatchImportByName("USER32.dll", "KillTimer",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateKillTimer),
                          patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnOKOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserOnCreateOnOK),
                             patches[1]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->m_hWnd = reinterpret_cast<HWND>(0x12345678);

    int result = 0;
    if (!installed) {
        result = 16;
    } else {
        g_netSessionBrowserOnCreateValidateResult = 0;
        dialog->m_selectedProviderIsModem = 0;
        dialog->m_shouldEnterHostSetup = 0;
        ResetNetSessionBrowserOnCreateLog();
        dialog->OnCreateSession();
        const bool invalidOk =
            g_netSessionBrowserOnCreateValidateCalls == 1 &&
            g_netSessionBrowserOnCreateValidateThis == dialog &&
            g_netSessionBrowserOnCreateKillTimerCalls == 0 &&
            g_netSessionBrowserOnCreateCreateSessionCalls == 0 &&
            g_netSessionBrowserOnCreateOnOkCalls == 0 &&
            dialog->m_shouldEnterHostSetup == 0;

        g_netSessionBrowserOnCreateValidateResult = 1;
        dialog->m_selectedProviderIsModem = 0;
        dialog->m_shouldEnterHostSetup = 0;
        ResetNetSessionBrowserOnCreateLog();
        dialog->OnCreateSession();
        const bool hostSetupOk =
            g_netSessionBrowserOnCreateValidateCalls == 1 &&
            g_netSessionBrowserOnCreateKillTimerCalls == 1 &&
            g_netSessionBrowserOnCreateKillTimerHwnd == dialog->m_hWnd &&
            g_netSessionBrowserOnCreateKillTimerId == 2 &&
            g_netSessionBrowserOnCreateCreateSessionCalls == 0 &&
            g_netSessionBrowserOnCreateOnOkCalls == 1 &&
            g_netSessionBrowserOnCreateOnOkThis == dialog &&
            dialog->m_shouldEnterHostSetup == 1;

        g_netSessionBrowserOnCreateValidateResult = 1;
        g_netSessionBrowserOnCreateCreateSessionResult = 0;
        dialog->m_selectedProviderIsModem = 1;
        dialog->m_shouldEnterHostSetup = 0;
        ResetNetSessionBrowserOnCreateLog();
        dialog->OnCreateSession();
        const bool modemFailureFieldsOk =
            g_netSessionBrowserOnCreateStatusFields.eventCode == 256 &&
            g_netSessionBrowserOnCreateStatusFields.statusFlags == 0 &&
            g_netSessionBrowserOnCreateStatusFields.valueOrTime == 10 &&
            g_netSessionBrowserOnCreateStatusFields.auxParam == 10 &&
            g_netSessionBrowserOnCreateStatusFields.maxPlayers == 2 &&
            std::strcmp(g_netSessionBrowserOnCreateStatusFields.sessionNameBuf,
                        "ModemSession") == 0;
        const bool modemFailureOk =
            g_netSessionBrowserOnCreateValidateCalls == 1 &&
            g_netSessionBrowserOnCreateKillTimerCalls == 0 &&
            g_netSessionBrowserOnCreateCreateSessionCalls == 1 &&
            modemFailureFieldsOk &&
            g_netSessionBrowserOnCreateSetNetworkCalls == 0 &&
            g_netSessionBrowserOnCreateSetModemCalls == 0 &&
            g_netSessionBrowserOnCreateGetPlayerNameCalls == 0 &&
            g_netSessionBrowserOnCreateCreateLocalPlayerCalls == 0 &&
            g_netSessionBrowserOnCreateOnOkCalls == 0 &&
            dialog->m_shouldEnterHostSetup == 0;

        g_netSessionBrowserOnCreateValidateResult = 1;
        g_netSessionBrowserOnCreateCreateSessionResult = 1;
        dialog->m_selectedProviderIsModem = 1;
        dialog->m_shouldEnterHostSetup = 0;
        ResetNetSessionBrowserOnCreateLog();
        dialog->OnCreateSession();
        const bool modemSuccessOk =
            g_netSessionBrowserOnCreateValidateCalls == 1 &&
            g_netSessionBrowserOnCreateKillTimerCalls == 0 &&
            g_netSessionBrowserOnCreateCreateSessionCalls == 1 &&
            g_netSessionBrowserOnCreateStatusFields.eventCode == 256 &&
            g_netSessionBrowserOnCreateStatusFields.statusFlags == 0 &&
            g_netSessionBrowserOnCreateStatusFields.valueOrTime == 10 &&
            g_netSessionBrowserOnCreateStatusFields.auxParam == 10 &&
            g_netSessionBrowserOnCreateStatusFields.maxPlayers == 2 &&
            std::strcmp(g_netSessionBrowserOnCreateStatusFields.sessionNameBuf,
                        "ModemSession") == 0 &&
            g_netSessionBrowserOnCreateSetNetworkCalls == 1 &&
            g_netSessionBrowserOnCreateSetNetworkValue == 1 &&
            g_netSessionBrowserOnCreateSetModemCalls == 1 &&
            g_netSessionBrowserOnCreateSetModemValue == 1 &&
            g_netSessionBrowserOnCreateGetPlayerNameCalls == 1 &&
            g_netSessionBrowserOnCreateCreateLocalPlayerCalls == 1 &&
            g_netSessionBrowserOnCreateCreateLocalPlayerName != nullptr &&
            std::strcmp(g_netSessionBrowserOnCreateCreateLocalPlayerName, "Pilot") == 0 &&
            g_netSessionBrowserOnCreateOnOkCalls == 1 &&
            g_netSessionBrowserOnCreateOnOkThis == dialog &&
            dialog->m_shouldEnterHostSetup == 1;

        result = invalidOk && hostSetupOk && modemFailureOk && modemSuccessOk ? 0 : 17;
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    RestoreFunctionPatch(createLocalPlayerPatch);
    RestoreFunctionPatch(getPlayerNamePatch);
    RestoreFunctionPatch(setModemPatch);
    RestoreFunctionPatch(setNetworkPatch);
    RestoreFunctionPatch(createSessionPatch);
    RestoreFunctionPatch(validatePatch);
    ::operator delete(dialog);
    return result;
}

extern "C" int netui_verify_winsock2_or_prompt_continue_smoke(void) {
    const WORD kWs2WsaStartupOrdinal = 115;
    const WORD kWs2WsaCleanupOrdinal = 116;
    ImportFunctionPatch patches[5] = {};
    const bool installed =
        PatchImportByOrdinal("WS2_32.dll", kWs2WsaStartupOrdinal,
                             reinterpret_cast<void *>(&FakeNetUiWsaStartup),
                             patches[0]) &&
        PatchImportByOrdinal("WS2_32.dll", kWs2WsaCleanupOrdinal,
                             reinterpret_cast<void *>(&FakeNetUiWsaCleanup),
                             patches[1]) &&
        PatchImportByName("USER32.dll", "MessageBeep",
                          reinterpret_cast<void *>(&FakeNetUiMessageBeep),
                          patches[2]) &&
        PatchImportByName("USER32.dll", "GetFocus",
                          reinterpret_cast<void *>(&FakeNetUiGetFocus),
                          patches[3]) &&
        PatchImportByName("USER32.dll", "MessageBoxA",
                          reinterpret_cast<void *>(&FakeNetUiMessageBoxA),
                          patches[4]);

    int result = 0;
    if (!installed) {
        result = 10;
    } else {
        g_netUiWsaStartupResult = 0;
        g_netUiWsaStartupHighVersion = 2;
        g_netUiMessageBoxResult = IDNO;
        g_netUiGetFocusResult = reinterpret_cast<HWND>(0x2468);
        ResetNetUiWinsockPromptLog();
        const int okResult =
            NetUi::VerifyWinsock2OrPromptContinue("Network", "Need %u.%u");
        const bool successOk =
            okResult == 1 &&
            g_netUiWsaStartupCalls == 1 &&
            g_netUiWsaStartupVersion == 2 &&
            g_netUiWsaCleanupCalls == 0 &&
            g_netUiMessageBeepCalls == 0 &&
            g_netUiMessageBoxCalls == 0;

        g_netUiWsaStartupResult = 0;
        g_netUiWsaStartupHighVersion = 0x0101;
        g_netUiMessageBoxResult = IDNO;
        ResetNetUiWinsockPromptLog();
        const int mismatchNoResult =
            NetUi::VerifyWinsock2OrPromptContinue("Network", "Need %u.%u");
        const bool mismatchNoOk =
            mismatchNoResult == 0 &&
            g_netUiWsaStartupCalls == 1 &&
            g_netUiWsaCleanupCalls == 1 &&
            g_netUiMessageBeepCalls == 1 &&
            g_netUiMessageBeepType == MB_ICONEXCLAMATION &&
            g_netUiGetFocusCalls == 1 &&
            g_netUiMessageBoxCalls == 1 &&
            g_netUiMessageBoxHwnd == g_netUiGetFocusResult &&
            std::strcmp(g_netUiMessageBoxText, "Need 1.1") == 0 &&
            std::strcmp(g_netUiMessageBoxCaption, "Network") == 0 &&
            g_netUiMessageBoxType == (MB_ICONQUESTION | MB_YESNO);

        g_netUiWsaStartupResult = 0;
        g_netUiWsaStartupHighVersion = 0x0101;
        g_netUiMessageBoxResult = IDYES;
        ResetNetUiWinsockPromptLog();
        const int mismatchYesResult =
            NetUi::VerifyWinsock2OrPromptContinue("Network", "Need %u.%u");
        const bool mismatchYesOk =
            mismatchYesResult == 1 &&
            g_netUiWsaCleanupCalls == 1 &&
            g_netUiMessageBoxCalls == 1;

        g_netUiWsaStartupResult = 99;
        g_netUiWsaStartupHighVersion = 1;
        g_netUiMessageBoxResult = IDNO;
        ResetNetUiWinsockPromptLog();
        const int startupFailResult =
            NetUi::VerifyWinsock2OrPromptContinue("Network", "Need %u.%u");
        const bool startupFailOk =
            startupFailResult == 0 &&
            g_netUiWsaStartupCalls == 1 &&
            g_netUiWsaCleanupCalls == 0 &&
            g_netUiMessageBoxCalls == 1 &&
            std::strcmp(g_netUiMessageBoxText, "Need 1.0") == 0;

        result = successOk && mismatchNoOk && mismatchYesOk && startupFailOk ? 0 : 11;
    }

    for (int index = 4; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    return result;
}

extern "C" int znetwork_dplay_create_interface_and_coinitialize_smoke(void) {
    ImportFunctionPatch importPatches[2] = {};
    CodeFunctionPatch reportErrorPatch = {};
    CodeFunctionPatch reportOldPatch = {};
    const bool installed =
        PatchImportByName("ole32.dll", "CoInitialize",
                          reinterpret_cast<void *>(&FakeDPlayCreateCoInitialize),
                          importPatches[0]) &&
        PatchImportByName("ole32.dll", "CoCreateInstance",
                          reinterpret_cast<void *>(&FakeDPlayCreateCoCreateInstance),
                          importPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay_ReportError),
                          reinterpret_cast<void *>(&FakeDPlayCreateReportError),
                          reportErrorPatch) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zError::ReportOld),
                          reinterpret_cast<void *>(&FakeDPlayCreateReportOld),
                          reportOldPatch);

    int result = 0;
    if (!installed) {
        result = 10;
    } else {
        zNetwork_DPlay4 fakeDPlay = {};
        zNetwork_DPlay4 *outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x11111111);
        g_dplayCreateCoCreateResult = S_OK;
        g_dplayCreateCoCreateOut = &fakeDPlay;
        ResetDPlayCreateInterfaceLog();
        const int successResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool successOk =
            successResult == S_OK &&
            outDPlay == &fakeDPlay &&
            g_dplayCreateCoInitializeCalls == 1 &&
            g_dplayCreateCoInitializeReserved == nullptr &&
            g_dplayCreateCoCreateCalls == 1 &&
            g_dplayCreateCoCreateArgsOk &&
            g_dplayCreateCoCreateInitialOut == nullptr &&
            g_dplayCreateReportOldCalls == 0 &&
            g_dplayCreateReportErrorCalls == 1 &&
            g_dplayCreateReportErrorHresult == S_OK &&
            g_dplayCreateReportErrorLine == 0x39a;

        outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x22222222);
        g_dplayCreateCoCreateResult = (HRESULT)(0x80040154);
        g_dplayCreateCoCreateOut = nullptr;
        ResetDPlayCreateInterfaceLog();
        const int classMissingResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool classMissingOk =
            classMissingResult == (int)(0x80040154) &&
            outDPlay == nullptr &&
            g_dplayCreateReportOldCalls == 1 &&
            g_dplayCreateReportOldFlags == 0x400 &&
            g_dplayCreateReportOldLine == 0x394 &&
            std::strcmp(g_dplayCreateReportOldMessage, "Class not registered") == 0 &&
            g_dplayCreateReportErrorCalls == 1 &&
            g_dplayCreateReportErrorHresult == (int)(0x80040154);

        outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x33333333);
        g_dplayCreateCoCreateResult = (HRESULT)(0x80040110);
        g_dplayCreateCoCreateOut = nullptr;
        ResetDPlayCreateInterfaceLog();
        const int classCannotResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool classCannotOk =
            classCannotResult == (int)(0x80040110) &&
            outDPlay == nullptr &&
            g_dplayCreateReportOldCalls == 1 &&
            g_dplayCreateReportOldLine == 0x396 &&
            std::strcmp(g_dplayCreateReportOldMessage, "Class cannot be created") == 0 &&
            g_dplayCreateReportErrorCalls == 1;

        outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x44444444);
        g_dplayCreateCoCreateResult = (HRESULT)(0x800401f0);
        g_dplayCreateCoCreateOut = nullptr;
        ResetDPlayCreateInterfaceLog();
        const int notInitResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool notInitOk =
            notInitResult == (int)(0x800401f0) &&
            outDPlay == nullptr &&
            g_dplayCreateReportOldCalls == 1 &&
            g_dplayCreateReportOldLine == 0x398 &&
            std::strcmp(g_dplayCreateReportOldMessage, "CoCreate not initialized") == 0 &&
            g_dplayCreateReportErrorCalls == 0;

        outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x55555555);
        g_dplayCreateCoCreateResult = E_FAIL;
        g_dplayCreateCoCreateOut = nullptr;
        ResetDPlayCreateInterfaceLog();
        const int genericFailResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool genericFailOk =
            genericFailResult == E_FAIL &&
            outDPlay == nullptr &&
            g_dplayCreateReportOldCalls == 0 &&
            g_dplayCreateReportErrorCalls == 1 &&
            g_dplayCreateReportErrorHresult == E_FAIL &&
            g_dplayCreateReportErrorLine == 0x39a;

        outDPlay = reinterpret_cast<zNetwork_DPlay4 *>(0x66666666);
        g_dplayCreateCoCreateResult = E_FAIL;
        g_dplayCreateCoCreateOut = &fakeDPlay;
        ResetDPlayCreateInterfaceLog();
        g_dplayCreateCoCreateWriteOut = false;
        const int noWriteFailResult =
            zNetwork_DPlay::CreateInterfaceAndCoInitialize(&outDPlay);
        const bool noWriteFailOk =
            noWriteFailResult == E_FAIL &&
            outDPlay == nullptr &&
            g_dplayCreateCoCreateInitialOut == nullptr &&
            g_dplayCreateReportOldCalls == 0 &&
            g_dplayCreateReportErrorCalls == 1 &&
            g_dplayCreateReportErrorHresult == E_FAIL &&
            g_dplayCreateReportErrorLine == 0x39a;

        result = successOk && classMissingOk && classCannotOk && notInitOk &&
                         genericFailOk && noWriteFailOk
                     ? 0
                     : 11;
    }

    RestoreFunctionPatch(reportOldPatch);
    RestoreFunctionPatch(reportErrorPatch);
    RestoreImportPatch(importPatches[1]);
    RestoreImportPatch(importPatches[0]);
    return result;
}

extern "C" int znetwork_dplay_select_service_provider_and_init_connection_smoke(void) {
    CodeFunctionPatch patches[4] = {};
    const bool installed =
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork_DPlay::CloseReleaseAndCoUninitialize),
            reinterpret_cast<void *>(&FakeDPlaySelectClose), patches[0]) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetwork_DPlay::CreateInterfaceAndCoInitialize),
            reinterpret_cast<void *>(&FakeDPlaySelectCreate), patches[1]) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetworkDPlay::InitializeConnectionFromProviderInfo),
            reinterpret_cast<void *>(&FakeDPlaySelectInitialize), patches[2]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zNetwork_DPlay_ReportError),
                          reinterpret_cast<void *>(&FakeDPlaySelectReportError),
                          patches[3]);

    int result = 0;
    zNetwork_DPlay4 oldDirectPlay = {};
    zNetwork_DPlay4 newDirectPlay = {};
    char connectionData[8] = {};
    char tcpModemName[] = "TCP/IP Modem Provider";
    char serialName[] = "Serial Cable";
    zNetworkDPlayServiceProviderInfo provider = {};

    if (!installed) {
        result = 10;
    } else {
        provider.displayName = tcpModemName;
        provider.connectionData = nullptr;
        g_zNetwork_pDirectPlay4 = &oldDirectPlay;
        g_zNetwork_ActiveProviderIsModem = 5;
        g_zNetwork_ActiveProviderIsTcpIp = 6;
        g_zNetwork_TcpIpAsyncSendEnabled = 7;
        ResetDPlaySelectProviderLog();
        const int noConnectionResult =
            zNetworkDPlay::SelectServiceProviderAndInitConnection(&provider);
        const bool noConnectionOk =
            noConnectionResult == 0 &&
            g_dplaySelectCloseCalls == 1 &&
            g_dplaySelectCloseArg == &oldDirectPlay &&
            g_zNetwork_pDirectPlay4 == nullptr &&
            g_dplaySelectCreateCalls == 0 &&
            g_dplaySelectInitializeCalls == 0 &&
            g_zNetwork_ActiveProviderIsModem == 5 &&
            g_zNetwork_ActiveProviderIsTcpIp == 6 &&
            g_zNetwork_TcpIpAsyncSendEnabled == 7;

        provider.connectionData = connectionData;
        g_zNetwork_pDirectPlay4 = &oldDirectPlay;
        g_dplaySelectCreateResult = E_FAIL;
        g_dplaySelectCreateOut = nullptr;
        g_dplaySelectReportErrorResult = 44;
        ResetDPlaySelectProviderLog();
        const int createFailResult =
            zNetworkDPlay::SelectServiceProviderAndInitConnection(&provider);
        const bool createFailOk =
            createFailResult == 44 &&
            g_dplaySelectCloseCalls == 1 &&
            g_dplaySelectCreateCalls == 1 &&
            g_dplaySelectCreateOutPtr == &g_zNetwork_pDirectPlay4 &&
            g_dplaySelectInitializeCalls == 0 &&
            g_dplaySelectReportErrorCalls == 1 &&
            g_dplaySelectReportErrorHresult == E_FAIL &&
            g_dplaySelectReportErrorLine == 0x8f;

        g_zNetwork_pDirectPlay4 = &oldDirectPlay;
        g_dplaySelectCreateResult = S_OK;
        g_dplaySelectCreateOut = nullptr;
        ResetDPlaySelectProviderLog();
        const int nullCreateResult =
            zNetworkDPlay::SelectServiceProviderAndInitConnection(&provider);
        const bool nullCreateOk =
            nullCreateResult == 0 &&
            g_dplaySelectCreateCalls == 1 &&
            g_dplaySelectInitializeCalls == 0 &&
            g_dplaySelectReportErrorCalls == 0;

        g_zNetwork_pDirectPlay4 = &oldDirectPlay;
        g_dplaySelectCreateResult = S_OK;
        g_dplaySelectCreateOut = &newDirectPlay;
        g_dplaySelectInitializeResult = 77;
        g_zNetwork_ActiveProviderIsModem = 0;
        g_zNetwork_ActiveProviderIsTcpIp = 0;
        g_zNetwork_TcpIpAsyncSendEnabled = 0;
        provider.displayName = tcpModemName;
        ResetDPlaySelectProviderLog();
        const int tcpModemResult =
            zNetworkDPlay::SelectServiceProviderAndInitConnection(&provider);
        const bool tcpModemOk =
            tcpModemResult == 77 &&
            g_zNetwork_pDirectPlay4 == &newDirectPlay &&
            g_zNetwork_ActiveProviderIsModem == 1 &&
            g_zNetwork_ActiveProviderIsTcpIp == 1 &&
            g_zNetwork_TcpIpAsyncSendEnabled == 1 &&
            g_dplaySelectInitializeCalls == 1 &&
            g_dplaySelectInitializeProvider == &provider;

        g_zNetwork_pDirectPlay4 = &oldDirectPlay;
        g_dplaySelectCreateResult = S_OK;
        g_dplaySelectCreateOut = &newDirectPlay;
        g_dplaySelectInitializeResult = 12;
        provider.displayName = serialName;
        ResetDPlaySelectProviderLog();
        const int serialResult =
            zNetworkDPlay::SelectServiceProviderAndInitConnection(&provider);
        const bool serialOk =
            serialResult == 12 &&
            g_zNetwork_ActiveProviderIsModem == 0 &&
            g_zNetwork_ActiveProviderIsTcpIp == 0 &&
            g_zNetwork_TcpIpAsyncSendEnabled == 0 &&
            g_dplaySelectInitializeCalls == 1 &&
            g_dplaySelectInitializeProvider == &provider;

        result = noConnectionOk && createFailOk && nullCreateOk && tcpModemOk &&
                         serialOk
                     ? 0
                     : 11;
    }

    for (int index = 3; index >= 0; --index) {
        RestoreFunctionPatch(patches[index]);
    }
    g_zNetwork_pDirectPlay4 = nullptr;
    return result;
}

extern "C" int net_session_browser_dialog_connect_selected_provider_smoke(void) {
    ImportFunctionPatch importPatches[3] = {};
    CodeFunctionPatch functionPatches[6] = {};
    const bool installed =
        PatchImportByName("USER32.dll", "KillTimer",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectKillTimer),
                          importPatches[0]) &&
        PatchImportByName("USER32.dll", "SendMessageA",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectSendMessageA),
                          importPatches[1]) &&
        PatchImportByName("USER32.dll", "SetTimer",
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectSetTimer),
                          importPatches[2]) &&
        PatchFunctionJump(CWndEnableWindowAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectEnableWindow),
                          functionPatches[0]) &&
        PatchFunctionJump(CWndSetWindowTextAAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectSetWindowTextA),
                          functionPatches[1]) &&
        PatchFunctionJump(reinterpret_cast<void *>(&zLoc::GetMessageString),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserHelpGetMessageString),
                          functionPatches[2]) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&NetUi::VerifyWinsock2OrPromptContinue),
            reinterpret_cast<void *>(&FakeNetSessionBrowserConnectVerify),
            functionPatches[3]) &&
        PatchFunctionJump(
            reinterpret_cast<void *>(&zNetworkDPlay::SelectServiceProviderAndInitConnection),
            reinterpret_cast<void *>(&FakeNetSessionBrowserConnectSelectProvider),
            functionPatches[4]) &&
        PatchFunctionJump(NetSessionBrowserRefreshSessionListAddress(),
                          reinterpret_cast<void *>(&FakeNetSessionBrowserConnectRefresh),
                          functionPatches[5]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->m_hWnd = reinterpret_cast<HWND>(0x1001);
    dialog->m_okButton.m_hWnd = reinterpret_cast<HWND>(0x1002);
    dialog->m_createSessionButton.m_hWnd = reinterpret_cast<HWND>(0x1003);
    dialog->m_sessionList.m_hWnd = reinterpret_cast<HWND>(0x1004);
    dialog->m_providerCombo.m_hWnd = reinterpret_cast<HWND>(0x1005);

    char tcpName[] = "TCP/IP Provider";
    char modemName[] = "Modem Provider";
    zNetworkDPlayServiceProviderInfo provider = {};

    int result = 0;
    if (!installed) {
        result = 10;
    } else {
        g_netSessionBrowserConnectSelectedIndex = 3;
        g_netSessionBrowserConnectItemData = nullptr;
        ResetNetSessionBrowserConnectLog();
        dialog->ConnectSelectedProvider();
        const bool nullProviderOk =
            g_netSessionBrowserConnectKillTimerCalls == 1 &&
            g_netSessionBrowserConnectKillTimerHwnd == dialog->m_hWnd &&
            g_netSessionBrowserConnectKillTimerId == 2 &&
            g_netSessionBrowserConnectEnableCalls == 2 &&
            g_netSessionBrowserConnectEnableThis[0] == &dialog->m_okButton &&
            g_netSessionBrowserConnectEnableValue[0] == FALSE &&
            g_netSessionBrowserConnectEnableThis[1] == &dialog->m_createSessionButton &&
            g_netSessionBrowserConnectEnableValue[1] == FALSE &&
            g_netSessionBrowserConnectSelectCalls == 0;

        provider.displayName = tcpName;
        g_NetUiTcpIpProviderWarningShown = 0;
        g_netSessionBrowserConnectItemData = &provider;
        g_netSessionBrowserConnectVerifyResult = 0;
        ResetNetSessionBrowserConnectLog();
        dialog->ConnectSelectedProvider();
        const bool tcpDeclineOk =
            g_NetUiTcpIpProviderWarningShown == 1 &&
            g_netSessionBrowserConnectVerifyCalls == 1 &&
            std::strcmp(g_netSessionBrowserConnectVerifyCaption, "Network") == 0 &&
            std::strcmp(g_netSessionBrowserConnectVerifyFormat, "Need Winsock %u.%u") == 0 &&
            g_netSessionBrowserConnectSetCurSelCalls == 1 &&
            g_netSessionBrowserConnectSetCurSelWParam == 0 &&
            g_netSessionBrowserConnectEnableCalls == 2 &&
            g_netSessionBrowserConnectSelectCalls == 0;

        g_NetUiTcpIpProviderWarningShown = 0;
        g_netSessionBrowserConnectVerifyResult = 1;
        g_netSessionBrowserConnectRefreshResult = 2;
        ResetNetSessionBrowserConnectLog();
        dialog->m_selectedProviderIsModem = 9;
        dialog->ConnectSelectedProvider();
        const bool tcpAcceptOk =
            g_NetUiTcpIpProviderWarningShown == 1 &&
            g_netSessionBrowserConnectSelectCalls == 1 &&
            g_netSessionBrowserConnectSelectProvider == &provider &&
            g_netSessionBrowserConnectRefreshCalls == 1 &&
            g_netSessionBrowserConnectRefreshThis == dialog &&
            g_netSessionBrowserConnectSetTimerCalls == 1 &&
            g_netSessionBrowserConnectSetTimerHwnd == dialog->m_hWnd &&
            g_netSessionBrowserConnectSetTimerId == 2 &&
            g_netSessionBrowserConnectSetTimerMs == 1000 &&
            g_netSessionBrowserConnectSetTextCalls == 2 &&
            std::strcmp(g_netSessionBrowserConnectSetTextValue[0], "Join") == 0 &&
            std::strcmp(g_netSessionBrowserConnectSetTextValue[1], "Refresh") == 0 &&
            dialog->m_selectedProviderIsModem == 0;

        provider.displayName = modemName;
        g_NetUiTcpIpProviderWarningShown = 1;
        ResetNetSessionBrowserConnectLog();
        dialog->m_selectedProviderIsModem = 0;
        dialog->ConnectSelectedProvider();
        const bool modemOk =
            g_netSessionBrowserConnectVerifyCalls == 0 &&
            g_netSessionBrowserConnectSelectCalls == 1 &&
            g_netSessionBrowserConnectListResetCalls == 1 &&
            g_netSessionBrowserConnectRefreshCalls == 0 &&
            g_netSessionBrowserConnectSetTextCalls == 2 &&
            std::strcmp(g_netSessionBrowserConnectSetTextValue[0], "Dial") == 0 &&
            std::strcmp(g_netSessionBrowserConnectSetTextValue[1], "Host Modem") == 0 &&
            dialog->m_selectedProviderIsModem == 1;

        result = nullProviderOk && tcpDeclineOk && tcpAcceptOk && modemOk ? 0 : 11;
    }

    for (int index = 5; index >= 0; --index) {
        RestoreFunctionPatch(functionPatches[index]);
    }
    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_browser_dialog_do_data_exchange_smoke(void) {
    const WORD kMfc42DDVMaxCharsOrdinal = 2289;
    const WORD kMfc42DDXControlOrdinal = 2302;
    const WORD kMfc42DDXTextCStringOrdinal = 2370;
    ImportFunctionPatch patches[3] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXControlOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDXControl),
                             patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextCStringOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDXText),
                             patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDVMaxCharsOrdinal,
                             reinterpret_cast<void *>(&FakeNetSessionBrowserDDVMaxChars),
                             patches[2]);

    NetSessionBrowserDialog *const dialog =
        reinterpret_cast<NetSessionBrowserDialog *>(
            ::operator new(sizeof(NetSessionBrowserDialog)));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionBrowserDialog *const returned = dialog->Constructor(nullptr);
    unsigned char dataExchangeStorage[16] = {};
    CDataExchange *const dataExchange =
        reinterpret_cast<CDataExchange *>(dataExchangeStorage);
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        g_netSessionBrowserDdxStep = 0;
        for (int index = 0; index < 7; ++index) {
            g_netSessionBrowserDdxKind[index] = 0;
            g_netSessionBrowserDdxIdOrLimit[index] = 0;
            g_netSessionBrowserDdxContext[index] = nullptr;
            g_netSessionBrowserDdxValue[index] = nullptr;
        }

        dialog->NetSessionBrowserDialog::DoDataExchange(dataExchange);

        if (g_netSessionBrowserDdxStep != 7) {
            result = 4;
        } else if (g_netSessionBrowserDdxKind[0] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[0] != 1048 ||
                   g_netSessionBrowserDdxValue[0] != &dialog->m_playerNameEdit ||
                   g_netSessionBrowserDdxKind[1] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[1] != 1 ||
                   g_netSessionBrowserDdxValue[1] != &dialog->m_okButton ||
                   g_netSessionBrowserDdxKind[2] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[2] != 1030 ||
                   g_netSessionBrowserDdxValue[2] != &dialog->m_createSessionButton ||
                   g_netSessionBrowserDdxKind[3] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[3] != 1040 ||
                   g_netSessionBrowserDdxValue[3] != &dialog->m_sessionList ||
                   g_netSessionBrowserDdxKind[4] != 1 ||
                   g_netSessionBrowserDdxIdOrLimit[4] != 1114 ||
                   g_netSessionBrowserDdxValue[4] != &dialog->m_providerCombo ||
                   g_netSessionBrowserDdxKind[5] != 2 ||
                   g_netSessionBrowserDdxIdOrLimit[5] != 1048 ||
                   g_netSessionBrowserDdxValue[5] != &dialog->m_playerName ||
                   g_netSessionBrowserDdxKind[6] != 3 ||
                   g_netSessionBrowserDdxIdOrLimit[6] != 21 ||
                   g_netSessionBrowserDdxValue[6] != &dialog->m_playerName) {
            result = 5;
        } else {
            for (int index = 0; index < 7; ++index) {
                if (g_netSessionBrowserDdxContext[index] != dataExchange) {
                    result = 6;
                    break;
                }
            }
        }
    }

    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }

    dialog->m_playerName.~CString();
    ::operator delete(dialog);
    return result;
}

struct TestRemoteHudPanelOps {
    void SetPos(int x, int y) {
        ++g_remoteHudSetPosCount;
        g_remoteHudSetPosThis = reinterpret_cast<HudUiPanel *>(this);
        g_remoteHudLastX = x;
        g_remoteHudLastY = y;
    }

    void SetVisible(int visible) {
        ++g_remoteHudSetVisibleCount;
        g_remoteHudLastVisible = visible;
    }
};

struct TestSpawnRemoteHudPanelOps {
    void SetVisible(int visible) {
        ++g_spawnRemoteSetVisibleCount;
        g_spawnRemoteLastVisible = visible;
    }
};

zNetwork_DPlay4Vtable MakeDPlayVtable() {
    zNetwork_DPlay4Vtable vtable{};
    vtable.Send_68 = SendFake;
    vtable.SetSessionDesc_7c = SetSessionDescFake;
    return vtable;
}

const zNetwork_DPlay4Vtable kDPlayVtable = MakeDPlayVtable();

void ClearDispatchHandlerListForTest(zNetworkDispatchHandlerListNode &sentinel) {
    zNetworkDispatchHandlerListNode *node = sentinel.next;
    while (node != &sentinel) {
        zNetworkDispatchHandlerListNode *const next = node->next;
        ::operator delete(node->record);
        ::operator delete(node);
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}
} // namespace

extern "C" int gamenet_list_reset_smoke(void) {
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = reinterpret_cast<GameNetSpawnPoint *>(2);
    g_GameNetSpawnPointTail = reinterpret_cast<GameNetSpawnPoint *>(3);
    g_GameNetSpawnPointCount = 4;
    GameNetSpawnPointList::InitGlobals();
    if (g_GameNetSpawnPointList != 0 || g_GameNetSpawnPointHead != nullptr ||
        g_GameNetSpawnPointTail != nullptr || g_GameNetSpawnPointCount != 0) {
        return 1;
    }

    g_GameNetPlayerRowList = 5;
    g_GameNetPlayerRowHead = reinterpret_cast<GameNetPlayerRow *>(6);
    g_GameNetPlayerRowTail = reinterpret_cast<GameNetPlayerRow *>(7);
    g_GameNetPlayerRowCount = 8;
    GameNetPlayerRowList::Reset();
    return g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == nullptr &&
                   g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0
               ? 0
               : 2;
}

extern "C" int gamenet_player_row_append_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldList = g_GameNetPlayerRowList;
    const unsigned int oldCount = g_GameNetPlayerRowCount;

    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    GameNetPlayerRowListState *const list =
        (GameNetPlayerRowListState *)(&g_GameNetPlayerRowList);
    GameNetPlayerRow *const first = GameNetPlayerRowList::AppendNewRow(list, 0);
    GameNetPlayerRow *const second = GameNetPlayerRowList::AppendNewRow(list, 1);
    const bool ok = first != nullptr && second != nullptr && first != second &&
                    g_GameNetPlayerRowHead == first && g_GameNetPlayerRowTail == second &&
                    g_GameNetPlayerRowCount == 2 && first->next == second &&
                    second->next == nullptr && second->playerKey == 0 &&
                    second->displayName[0] == 0;

    ::operator delete(second);
    ::operator delete(first);
    g_GameNetPlayerRowList = oldList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    return ok ? 0 : 1;
}

extern "C" int gamenet_unregister_gameplay_packet_handlers_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;

    zNetworkDispatchHandlerListNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 1;

    zNetwork::RegisterPacketHandler(6, (zNetworkPacketHandler)&GameNet::HandlePkt06_PlayerStateSnapshot, 0);
    zNetwork::RegisterPacketHandler(7, (zNetworkPacketHandler)&GameNet::HandlePkt07_AltGunDispatch, 0);
    zNetwork::RegisterPacketHandler(
        0x0a, (zNetworkPacketHandler)&OptCatalog::HandlePkt0A_RemoveRuntimeRelay, 0);
    zNetwork::RegisterPacketHandler(1, (zNetworkPacketHandler)&GameNet::ReassignPlayerColorsAndRefreshRows, 0);
    zNetwork::RegisterPacketHandler(8, (zNetworkPacketHandler)&GameNet::HandlePkt08_PlayerKillEvent, 0);
    zNetwork::RegisterPacketHandler(9, (zNetworkPacketHandler)&GameNet::HandlePkt09_PlayerScoreboardSnapshot, 0);
    zNetwork::RegisterPacketHandler(0x0b, (zNetworkPacketHandler)&GameNet::HandlePkt0B_ChatMessage, 0);
    zNetwork::RegisterPacketHandler(0x0e, (zNetworkPacketHandler)&GameNet::HandlePkt0E_PlayerLapProgress, 0);
    zNetwork::RegisterPacketHandler(0x0c, (zNetworkPacketHandler)&GameNet::HandlePkt0C_HudTimerStatusBits, 0);
    zNetwork::RegisterPacketHandler(0x0d, (zNetworkPacketHandler)&GameNet::HandlePkt0D_HudTimerPanelState, 0);
    zNetwork::RegisterPacketHandler(0x0f, (zNetworkPacketHandler)&zDEClient_Crater::NetRelayCallback, 0);
    zNetwork::RegisterPacketHandler(0x10, (zNetworkPacketHandler)&zDEClient_QSand::NetRelayCallback, 0);
    zNetwork::RegisterPacketHandler(0x11, (zNetworkPacketHandler)&Pickup::HandlePkt11_SpawnDelta, 0);
    zNetwork::RegisterPacketHandler(
        0x12, (zNetworkPacketHandler)&Pickup::HandlePkt12_AirdropSpawnChuteRelay, 0);
    zNetwork::RegisterPacketHandler(
        0x13, (zNetworkPacketHandler)&GameNet::HandlePkt13_EffectAnimActivationRecord, 0);

    GameNet::UnregisterGameplayPacketHandlers();
    const bool ok = g_zNetwork_DispatchHandlerListCount == 0 && sentinel.next == &sentinel &&
                    sentinel.prev == &sentinel && g_GameNet_HandlersRegistered == 0;

    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;

    return ok ? 0 : 1;
}

extern "C" int gamenet_register_gameplay_handlers_and_callbacks_smoke(void) {
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldCount = g_zNetwork_DispatchHandlerListCount;
    const int oldRegistered = g_GameNet_HandlersRegistered;
    zDEClient_NetRelayCallback const oldCraterRelay = g_zDEClientCraterNetRelayCallback;
    zDEClient_NetRelayCallback const oldQSandRelay = g_zDEClientQSandNetRelayCallback;
    OptCatalogAllocRuntimeGateCallback const oldAllocGate = g_OptCatalog_AllocRuntimeGateCallback;
    OptCatalogAllocRuntimeGateCallback const oldNoOpGate =
        g_OptCatalog_AltGunDispatchNoOpCallback;
    OptCatalogRemoveRuntimeRelayCallback const oldRemoveRelay =
        g_OptCatalog_RemoveRuntimeRelayCallback;
    void(__fastcall *oldEffectDispatch)(zEffectAnimActivationRecord *) =
        g_zEffectAnim_ActivationDispatchCallback;
    const unsigned int oldEffectDispatchTag = g_zEffectAnim_ActivationDispatchTagHigh;

    zNetworkDispatchHandlerListNode sentinel{};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;
    g_GameNet_HandlersRegistered = 0;
    g_zDEClientCraterNetRelayCallback = nullptr;
    g_zDEClientQSandNetRelayCallback = nullptr;
    g_OptCatalog_AllocRuntimeGateCallback = nullptr;
    g_OptCatalog_AltGunDispatchNoOpCallback = nullptr;
    g_OptCatalog_RemoveRuntimeRelayCallback = nullptr;
    g_zEffectAnim_ActivationDispatchCallback = nullptr;
    g_zEffectAnim_ActivationDispatchTagHigh = 0;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();

    unsigned int packetMask = 0;
    bool modesOk = true;
    for (zNetworkDispatchHandlerListNode *node = sentinel.next; node != &sentinel;
         node = node->next) {
        if (node->record == nullptr || node->record->mode != 2) {
            modesOk = false;
            break;
        }
        if (node->record->packetType >= 0 && node->record->packetType < 32) {
            packetMask |= 1u << static_cast<unsigned int>(node->record->packetType);
        }
    }

    const unsigned int expectedMask =
        (1u << 1) | (1u << 3) | (1u << 6) | (1u << 7) | (1u << 8) |
        (1u << 9) | (1u << 0x0a) | (1u << 0x0b) | (1u << 0x0c) |
        (1u << 0x0d) | (1u << 0x0e) | (1u << 0x0f) | (1u << 0x10) |
        (1u << 0x11) | (1u << 0x12) | (1u << 0x13) | (1u << 0x14);
    const bool registeredOk = g_GameNet_HandlersRegistered == 1 &&
                              g_zNetwork_DispatchHandlerListCount == 17 && modesOk &&
                              packetMask == expectedMask;
    const bool callbacksOk =
        g_zDEClientCraterNetRelayCallback ==
            (zDEClient_NetRelayCallback)&zDEClient_Crater::Execute &&
        g_zDEClientQSandNetRelayCallback ==
            (zDEClient_NetRelayCallback)&GameNet::SendPkt10_QSandEvent &&
        g_OptCatalog_AllocRuntimeGateCallback ==
            &OptCatalog::AltGunDispatchAllocRuntimeGateCallback &&
        g_OptCatalog_AltGunDispatchNoOpCallback == &GameNet::AltGunDispatchNoOpCallback &&
        g_OptCatalog_RemoveRuntimeRelayCallback == &OptCatalog::SendPkt0A_RemoveRuntimeRelay &&
        g_zEffectAnim_ActivationDispatchCallback ==
            &GameNet::SendPkt13_EffectAnimActivationRecord &&
        g_zEffectAnim_ActivationDispatchTagHigh == 0x0c000000u;

    GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks();
    const bool noDuplicateOk = g_zNetwork_DispatchHandlerListCount == 17;

    ClearDispatchHandlerListForTest(sentinel);
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldCount;
    g_GameNet_HandlersRegistered = oldRegistered;
    g_zDEClientCraterNetRelayCallback = oldCraterRelay;
    g_zDEClientQSandNetRelayCallback = oldQSandRelay;
    g_OptCatalog_AllocRuntimeGateCallback = oldAllocGate;
    g_OptCatalog_AltGunDispatchNoOpCallback = oldNoOpGate;
    g_OptCatalog_RemoveRuntimeRelayCallback = oldRemoveRelay;
    g_zEffectAnim_ActivationDispatchCallback = oldEffectDispatch;
    g_zEffectAnim_ActivationDispatchTagHigh = oldEffectDispatchTag;

    return registeredOk && callbacksOk && noDuplicateOk ? 0 : 1;
}

extern "C" int gamenet_chat_compose_key_callback_smoke(void) {
    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldTableReady = g_zInput_KbdDikToAsciiTableReady;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel descPanel{};
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    g_HudUiMgrObjectiveChatComposeTextInput.buffer[0] = '\0';
    g_chatComposeSetTextFmtCalls = 0;
    g_chatComposeSetTextFmtThis = nullptr;
    g_chatComposeSetTextFmtText[0] = '\0';
    g_zInput_KbdDikToAsciiTableReady = 0;
    std::memset(g_zInput_KbdDikToAsciiTable, 0, sizeof(g_zInput_KbdDikToAsciiTable));

    GameNet::ChatComposeKeyCallback(0x41e);
    const bool inserted =
        std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "A") == 0 &&
        g_HudUiMgrObjectiveChatComposeTextInput.cursor == 1 &&
        g_chatComposeSetTextFmtCalls == 1 &&
        g_chatComposeSetTextFmtThis == &descPanel &&
        std::strcmp(g_chatComposeSetTextFmtText, "A") == 0;

    GameNet::ChatComposeKeyCallback(0);
    const bool zeroIgnored = g_chatComposeSetTextFmtCalls == 1 &&
                             std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "A") == 0;

    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_zInput_KbdDikToAsciiTableReady = oldTableReady;

    return inserted && zeroIgnored ? 0 : 1;
}

extern "C" int gamenet_begin_chat_compose_smoke(void) {
    int networkEnabled = 0;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    g_HudUiMgrObjectiveChatComposeActive = 77;
    GameNet::BeginChatCompose();
    const bool disabledOk = g_HudUiMgrObjectiveChatComposeActive == 77;

    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    const int oldShowReset = g_HudUiMgrObjectiveShowResetUnused;
    const float oldAutoHide = g_HudUiMgrObjectiveAutoHideDelaySec;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(oldDispatch, g_zInputKbdKeyDispatchTable, sizeof(oldDispatch));

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel summaryPanel{};
    HudUiPanel descPanel{};
    summaryPanel.vtbl = &panelTable;
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrObjectiveBar.ftable = (const HudUiCommon_FTable *)(&g_HudUiBar_FTable);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;
    g_chatComposeSetTextFmtCalls = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    networkEnabled = 1;
    GameNet::BeginChatCompose();
    const bool stateOk = g_HudUiMgrObjectiveChatComposeActive == 1 &&
                         g_HudUiMgrObjectiveState == 1 &&
                         g_HudUiMgrObjectivePhase == 1 &&
                         g_HudUiMgrObjectiveChatComposeTextInput.capacity == 32 &&
                         std::strcmp(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(), "") == 0 &&
                         g_zInput_BindMapOverlayDepth == 1;
    const void *const callback = reinterpret_cast<void *>(&GameNet::ChatComposeKeyCallback);
    const bool keyOk =
        g_zInputKbdKeyDispatchTable[0x02].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x402].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x0e].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x10].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x42b].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x1e].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x428].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x2c].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x435].callback == callback &&
        g_zInputKbdKeyDispatchTable[0x39].callback == callback;

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapContext_Pop();
    zInput::BindMapSystem_Shutdown();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectiveShowResetUnused = oldShowReset;
    g_HudUiMgrObjectiveAutoHideDelaySec = oldAutoHide;
    std::memcpy(g_zInputKbdKeyDispatchTable, oldDispatch, sizeof(oldDispatch));
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return disabledOk && stateOk && keyOk ? 0 : 1;
}

extern "C" int gamenet_end_chat_compose_and_send_smoke(void) {
    int networkEnabled = 1;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;
    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldOverlayDepth = g_zInput_BindMapOverlayDepth;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    GameNetPlayerRow row{};
    std::strcpy(row.displayName, "Pilot");
    zUtil_SaveGameState saveState{};
    saveState.netPlayerRow = &row;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);

    HudUiChatMessageStack chat{};
    chat.Constructor();
    chat.base.enabled = 1;
    g_HudUiChatMessageStack = &chat;

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel summaryPanel{};
    HudUiPanel descPanel{};
    summaryPanel.vtbl = &panelTable;
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrObjectiveBar.ftable = (const HudUiCommon_FTable *)(&g_HudUiBar_FTable);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);
    GameNet::BeginChatCompose();
    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("go");

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::EndChatComposeAndSendThunk();

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&chat.lines[0][0]);
    const NetPkt0B_ChatMessage *const sentPacket =
        reinterpret_cast<const NetPkt0B_ChatMessage *>(g_sendPacketBytes);
    const bool sent = g_HudUiMgrObjectiveChatComposeActive == 0 &&
                      g_zInput_BindMapOverlayDepth == 0 && g_sendCalls == 1 &&
                      g_sendFlags == 1 && g_sendPacketSize == 20 &&
                      sentPacket->header.packetType == 0x0b &&
                      sentPacket->header.packetSizeBytes == 20 &&
                      sentPacket->header.payloadDword0 == 0x10203040 &&
                      sentPacket->messageLength == 8 &&
                      std::memcmp(sentPacket->message, "Pilot:go", 8) == 0 &&
                      std::strcmp(firstLine->GetLastTextPtr(), "Pilot:go") == 0 &&
                      FieldAt<float>(firstLine, 0x10) == 5.0f;

    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("");
    g_HudUiMgrObjectiveChatComposeActive = 1;
    zInput::BindMapContext_Push(0);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::EndChatComposeAndSend();

    const bool emptySkipped = g_HudUiMgrObjectiveChatComposeActive == 0 &&
                              g_zInput_BindMapOverlayDepth == 0 && g_sendCalls == 0 &&
                              std::strcmp(firstLine->GetLastTextPtr(), "Pilot:go") == 0;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&chat.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapSystem_Shutdown();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_zInput_BindMapOverlayDepth = oldOverlayDepth;
    g_HudUiChatMessageStack = oldChatStack;
    g_GameStateOrMapTable = oldGameState;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return sent && emptySkipped ? 0 : 1;
}

extern "C" int hud_ui_handle_hotkey_command_begin_chat_smoke(void) {
    int networkEnabled = 1;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    HudUiTextInput oldInput = g_HudUiMgrObjectiveChatComposeTextInput;
    HudUiPanel *const oldSummaryPanel = g_HudUiMgrObjectiveSummaryTextPanel;
    HudUiPanel *const oldDescPanel = g_HudUiMgrObjectiveDescTextPanel;
    const int oldChatComposeActive = g_HudUiMgrObjectiveChatComposeActive;
    const int oldPhase = g_HudUiMgrObjectivePhase;
    const int oldState = g_HudUiMgrObjectiveState;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(oldDispatch, g_zInputKbdKeyDispatchTable, sizeof(oldDispatch));

    HudUiPanel_FTable panelTable{};
    panelTable.slots[0x74 / 4] = reinterpret_cast<unsigned int>(&ChatComposeSetTextFmtFake);
    HudUiPanel summaryPanel{};
    HudUiPanel descPanel{};
    summaryPanel.vtbl = &panelTable;
    descPanel.vtbl = &panelTable;
    g_HudUiMgrObjectiveSummaryTextPanel = &summaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = &descPanel;
    g_HudUiMgrObjectiveWidget.ftable = &g_HudUiWidget_FTable;
    g_HudUiMgrObjectiveSensorRect.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrSensorOverlay.ftable =
        reinterpret_cast<const HudUiWidget_FTable *>(&g_HudUiWidget_FTable);
    g_HudUiMgrObjectiveBar.ftable = (const HudUiCommon_FTable *)(&g_HudUiBar_FTable);
    g_HudUiMgrObjectivePhase = 0;
    g_HudUiMgrObjectiveState = 0;
    g_HudUiMgrObjectiveChatComposeActive = 0;

    g_HudUiMgrObjectiveChatComposeTextInput = {};
    g_HudUiMgrObjectiveChatComposeTextInput.Constructor(8);
    char *const initialBuffer = g_HudUiMgrObjectiveChatComposeTextInput.buffer;
    zInput::BindMapSystem_Init(1);

    HudUi::HandleHotkeyCommand(42);
    const void *const callback = reinterpret_cast<void *>(&GameNet::ChatComposeKeyCallback);
    const bool hotkeyOk = g_HudUiMgrObjectiveChatComposeActive == 1 &&
                          g_HudUiMgrObjectiveChatComposeTextInput.capacity == 32 &&
                          g_zInput_BindMapOverlayDepth == 1 &&
                          g_zInputKbdKeyDispatchTable[0x39].callback == callback &&
                          g_zInputKbdKeyDispatchTable[0x42b].callback == callback;

    ::operator delete(initialBuffer);
    g_HudUiMgrObjectiveChatComposeTextInput.DestructorCore();
    zInput::BindMapContext_Pop();
    zInput::BindMapSystem_Shutdown();

    g_HudUiMgrObjectiveChatComposeTextInput = oldInput;
    g_HudUiMgrObjectiveSummaryTextPanel = oldSummaryPanel;
    g_HudUiMgrObjectiveDescTextPanel = oldDescPanel;
    g_HudUiMgrObjectiveChatComposeActive = oldChatComposeActive;
    g_HudUiMgrObjectivePhase = oldPhase;
    g_HudUiMgrObjectiveState = oldState;
    std::memcpy(g_zInputKbdKeyDispatchTable, oldDispatch, sizeof(oldDispatch));
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;

    return hotkeyOk ? 0 : 1;
}

extern "C" int hud_timer_panel_net_state_clear_tail_flags_smoke(void) {
    HudTimerPanelNetState state{};
    for (std::uint32_t &flag : state.tailFlags) {
        flag = 0xffffffff;
    }

    state.ClearTailFlagsLocal();
    for (std::uint32_t flag : state.tailFlags) {
        if (flag != 0) {
            return 1;
        }
    }

    return 0;
}

extern "C" int gamenet_find_player_row_and_status_bits_smoke(void) {
    GameNetPlayerRow first{};
    GameNetPlayerRow second{};
    first.playerKey = 10;
    first.lapCount = 3;
    first.next = &second;
    second.playerKey = 20;
    second.lapCount = 4;
    g_GameNetPlayerRowHead = &first;
    g_HudSensorTracker.runtimeGoalValue = 3;

    const bool rowLookup =
        GameNet::FindPlayerRowByKey(20) == &second && GameNet::FindPlayerRowByKey(30) == nullptr;
    const bool lapsReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    second.lapCount = 2;
    const bool lapsBlocked =
        GameNet::AreAllPlayersAtLapTarget() == 0 && g_GameNetAllPlayersLapTargetCheckStarted == 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 0;
    g_GameNetPlayerRowHead = nullptr;
    const bool emptyListReached =
        GameNet::AreAllPlayersAtLapTarget() == 1 && g_GameNetAllPlayersLapTargetCheckStarted == 1;

    GameNet::SetStatusBitsFromFlags(3);
    const bool bothSet = g_GameNetStatus_AllowMaps == 1 && g_GameNetStatus_NameTags == 1 &&
                         GameNet::GetStatusBitAllowMaps() == 1 &&
                         GameNet::GetStatusBitNameTags() == 1;

    GameNet::SetStatusBitsFromFlags(0);
    const bool bothClear = g_GameNetStatus_AllowMaps == 0 && g_GameNetStatus_NameTags == 0 &&
                           GameNet::GetStatusBitAllowMaps() == 0 &&
                           GameNet::GetStatusBitNameTags() == 0;

    g_GameNetPlayerRowHead = nullptr;
    return rowLookup && lapsReached && lapsBlocked && emptyListReached && bothSet && bothClear ? 0
                                                                                               : 1;
}

extern "C" int gamenet_update_remote_player_hud_widget_screen_pos_smoke(void) {
    const int oldNameTags = g_GameNetStatus_NameTags;
    int *const oldReplicateOption = ZOPT_REPLICATE;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeDiScene = g_Player_RuntimeDiScene;
    int *const oldMatrixIdentitySlot = zMath::g_currentMatrixIdentityFlagSlot;
    float **const oldMatrixPtrSlot = zMath::g_currentMatrixPtrSlot;

    int replicateMode = 0;
    ZOPT_REPLICATE = &replicateMode;

    int matrixIdentityFlags[2] = {};
    float *matrixSlots[2] = {};
    zMat4x3 baseMatrix = {};
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

    HudUiPanel_FTable panelTable = {};
    panelTable.slots[0x0c / 4] = MethodAddress(&TestRemoteHudPanelOps::SetPos);
    panelTable.slots[0x60 / 4] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);

    zClass_NodePartial localRoot = {};
    zClass_NodePartial remoteRoot = {};
    zUtil_PlayerStateStorage localPlayer = {};
    localPlayer.rootNode = &localRoot;
    zUtil_SaveGameState localSave = {};
    localSave.playerState = &localPlayer;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSave);
    g_Player_RuntimeDiScene = nullptr;

    zUtil_PlayerStateStorage remotePlayer = {};
    remotePlayer.rootNode = &remoteRoot;
    remotePlayer.worldPos = {1.0f, 2.0f, 10.0f};
    zUtil_SaveGameState remoteSave = {};
    remoteSave.playerState = &remotePlayer;
    GameNetPlayerRow row = {};
    row.hudWidget.vtbl = &panelTable;
    FieldAt<int>(&row.hudWidget, 0x260) = 14;
    FieldAt<int>(&row.hudWidget, 0x270) = 0;
    FieldAt<int>(&row.hudWidget, 0x274) = 0;
    remoteSave.netPlayerRow = &row;

    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    g_GameNetStatus_NameTags = 0;
    const bool disabledOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                            g_remoteHudSetVisibleCount == 0 && g_remoteHudSetPosCount == 0;

    g_GameNetStatus_NameTags = 1;
    const int visibleResult = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave);
    const bool visibleOk =
        visibleResult == 1 && g_remoteHudSetPosCount == 1 && g_remoteHudSetPosThis == &row.hudWidget &&
        g_remoteHudLastX == 330 && g_remoteHudLastY == 205 &&
        g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 1;

    replicateMode = 1;
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const int replicateResult = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave);
    const bool replicateOk = replicateResult == 1 && g_remoteHudLastX == 660 &&
                             g_remoteHudLastY == 420 && g_remoteHudLastVisible == 1;

    replicateMode = 0;
    remotePlayer.worldPos = {1.0f, 38.0f, 10.0f};
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const bool marginHideOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                              g_remoteHudSetVisibleCount == 1 &&
                              g_remoteHudLastVisible == 0 && g_remoteHudSetPosCount == 0;

    remotePlayer.worldPos = {-100.0f, 2.0f, 10.0f};
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudSetPosCount = 0;
    const bool clippedHideOk = GameNet::UpdateRemotePlayerHudWidgetScreenPos(&remoteSave) == 0 &&
                               g_remoteHudSetVisibleCount == 1 &&
                               g_remoteHudLastVisible == 0 && g_remoteHudSetPosCount == 0;

    g_GameNetStatus_NameTags = oldNameTags;
    ZOPT_REPLICATE = oldReplicateOption;
    g_GameStateOrMapTable = oldGameState;
    g_Player_RuntimeDiScene = oldRuntimeDiScene;
    zMath::g_currentMatrixIdentityFlagSlot = oldMatrixIdentitySlot;
    zMath::g_currentMatrixPtrSlot = oldMatrixPtrSlot;

    return disabledOk && visibleOk && replicateOk && marginHideOk && clippedHideOk ? 0 : 1;
}

extern "C" int gamenet_get_local_player_color_index_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;

    g_GameStateOrMapTable = nullptr;
    const bool nullStateOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    zUtil_SaveGameState saveState{};
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)(&saveState);
    const bool nullRowOk = GameNet::GetLocalPlayerColorIndexOrZero() == 0;

    GameNetPlayerRow row{};
    row.playerColorIndex = 6;
    saveState.netPlayerRow = &row;
    const bool colorOk = GameNet::GetLocalPlayerColorIndexOrZero() == 6;

    g_GameStateOrMapTable = oldGameState;
    return nullStateOk && nullRowOk && colorOk ? 0 : 1;
}

extern "C" int gamenet_get_nearest_other_player_distance_to_spawn_point_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const unsigned int oldCount = g_GameNetPlayerRowCount;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    GameNetSpawnPoint spawnPoint = {};
    spawnPoint.position.x = 0.0f;
    spawnPoint.position.y = 0.0f;
    spawnPoint.position.z = 0.0f;

    zUtil_PlayerStateStorage localPlayerState = {};
    zUtil_PlayerStateStorage farPlayerState = {};
    zUtil_PlayerStateStorage nearPlayerState = {};
    localPlayerState.worldPos.x = 1.0f;
    farPlayerState.worldPos.x = 5.0f;
    nearPlayerState.worldPos.x = 2.0f;

    GameNetPlayerSaveState localSave = {};
    GameNetPlayerSaveState farSave = {};
    GameNetPlayerSaveState nearSave = {};
    localSave.playerState = &localPlayerState;
    farSave.playerState = &farPlayerState;
    nearSave.playerState = &nearPlayerState;

    GameNetPlayerRow localRow = {};
    GameNetPlayerRow farRow = {};
    GameNetPlayerRow nearRow = {};
    localRow.saveState = &localSave;
    localRow.next = &farRow;
    farRow.saveState = &farSave;
    farRow.next = &nearRow;
    nearRow.saveState = &nearSave;

    g_GameNetPlayerRowHead = &localRow;
    g_GameNetPlayerRowTail = &nearRow;
    g_GameNetPlayerRowCount = 3;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&localSave);

    GameNetPlayerSaveState *nearest = &localSave;
    const float nearestDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool nearestOk = nearestDistance == 4.0f && nearest == &nearSave;

    g_GameNetPlayerRowHead = nullptr;
    nearest = &localSave;
    const float emptyDistance =
        GameNet::GetNearestOtherPlayerDistanceToSpawnPoint(&spawnPoint, &nearest);
    const bool emptyOk = emptyDistance > 9.0e22f && nearest == &localSave;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    return nearestOk && emptyOk ? 0 : 1;
}

extern "C" int gamenet_respawn_player_color_indexed_spawn_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const std::uint32_t oldSpawnCount = g_GameNetSpawnPointCount;
    const std::int32_t oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldRaceCheckpointMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldMissionId = g_HudSensorTracker.missionId;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 20, 30, 160, 120,
                                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    GameNetPlayerRow localRow = {};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &localRow;
    modalState.masterModalData = &modalData;
    modalData.masterType = 3;
    localRow.playerColorIndex = 2;

    GameNetSpawnPoint firstSpawn = {};
    GameNetSpawnPoint secondSpawn = {};
    firstSpawn.position = {1.0f, 2.0f, 3.0f};
    firstSpawn.yawDegrees = 10.0f;
    firstSpawn.next = &secondSpawn;
    secondSpawn.position = {4.0f, 5.0f, 6.0f};
    secondSpawn.yawDegrees = 90.0f;
    g_GameNetSpawnPointHead = &firstSpawn;
    g_GameNetSpawnPointTail = &secondSpawn;
    g_GameNetSpawnPointCount = 2;
    g_GameNetStatus_AllowMaps = 0;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_HudSensorTracker.missionId = 11;

    playerState.throttleInput = 1.0f;
    playerState.steeringInput = 2.0f;
    playerState.subVerticalInput = 3.0f;
    playerState.throttleInputCopy = 4.0f;
    playerState.steeringInputCopy = 5.0f;
    playerState.subVerticalInputCopy = 6.0f;
    playerState.localVel = {7.0f, 8.0f, 9.0f};
    playerState.projectileSpawnVel = {10.0f, 11.0f, 12.0f};
    playerState.yawRotatedLocalVel = {13.0f, 14.0f, 15.0f};
    playerState.angVelPitch = 16.0f;
    playerState.angVelYaw = 17.0f;
    playerState.angVelRoll = 18.0f;
    playerState.thirdPersonYawOffset = 19.0f;
    playerState.cameraElevationOffset = 20.0f;
    playerState.amphibUnlocked = 0;
    playerState.hoverUnlocked = 1;
    playerState.subUnlocked = 1;

    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 32;
    g_zInput_MouseClientCenterY = 24;
    g_zInput_MouseStateSnapshot.cursorClientX = 4;
    g_zInput_MouseStateSnapshot.cursorClientY = 5;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.25f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.5f;

    GameNet::RespawnPlayerAndDropWeaponPickupIfAllowed(&saveState, 1);

    const bool spawnOk = Vec3Equals(playerState.worldPos, secondSpawn.position) &&
                         FloatNear(playerState.restartYawRad, 1.5707964f) &&
                         FloatNear(playerState.previousTransform.posX, 4.0f) &&
                         FloatNear(playerState.previousTransform.posY, 5.0f) &&
                         FloatNear(playerState.previousTransform.posZ, 6.0f);
    const bool resetOk =
        playerState.thirdPersonYawOffset == 0.0f && playerState.cameraElevationOffset == 0.0f &&
        Vec3Equals(playerState.localVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.projectileSpawnVel, {0.0f, 0.0f, 0.0f}) &&
        Vec3Equals(playerState.yawRotatedLocalVel, {0.0f, 0.0f, 0.0f}) &&
        playerState.angVelPitch == 0.0f && playerState.angVelYaw == 0.0f &&
        playerState.angVelRoll == 0.0f && playerState.throttleInput == 0.0f &&
        playerState.steeringInput == 0.0f && playerState.subVerticalInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.subVerticalInputCopy == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorClientX == 32 &&
        g_zInput_MouseStateSnapshot.cursorClientY == 24 &&
        g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
        g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;
    const bool unlockOk =
        playerState.amphibUnlocked == 1 && playerState.hoverUnlocked == 0 &&
        playerState.subUnlocked == 0;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameStateOrMapTable = oldGameState;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_HudSensorTracker.raceCheckpointMode = oldRaceCheckpointMode;
    g_HudSensorTracker.missionId = oldMissionId;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;

    return spawnOk && resetOk && unlockOk ? 0 : 2;
}

extern "C" int gamenet_reset_hud_timer_panel_net_state_smoke(void) {
    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;

    g_HudTimerPanelNetState.timerSeconds = 1.0f;
    g_HudTimerPanelNetState.timeWarningThresholdSec = 2.0f;
    g_HudTimerPanelNetState.timerDirectionNeg = 0;
    g_HudTimerPanelNetState.startGateTriggered = 1;
    g_HudTimerPanelNetState.raceFinishCountdownTriggered = 1;
    g_HudTimerPanelNetState.startCountdownTriggered = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    for (std::uint32_t &flag : g_HudTimerPanelNetState.tailFlags) {
        flag = 0xffffffff;
    }
    g_GameNetOneLapLeftMessageShown = 1;
    g_GameNetAllPlayersLapTargetCheckStarted = 1;

    GameNet::ResetHudTimerPanelNetStateLongCountdown();

    bool tailsCleared = true;
    for (std::uint32_t flag : g_HudTimerPanelNetState.tailFlags) {
        tailsCleared = tailsCleared && flag == 0;
    }

    const bool ok = g_HudTimerPanelNetState.timerSeconds == 36000.0f &&
                    g_HudTimerPanelNetState.timeWarningThresholdSec == 120.0f &&
                    g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                    g_HudTimerPanelNetState.startGateTriggered == 0 &&
                    g_HudTimerPanelNetState.raceFinishCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.startCountdownTriggered == 0 &&
                    g_HudTimerPanelNetState.tenSecondWarningsEnabled == 0 &&
                    g_GameNetOneLapLeftMessageShown == 0 &&
                    g_GameNetAllPlayersLapTargetCheckStarted == 0 && tailsCleared &&
                    FieldAt<float>(&timer, 0x2a4) == 36000.0f &&
                    FieldAt<std::int32_t>(&timer, 0x2ac) == -1 &&
                    std::strcmp(&FieldAt<char>(&timer, 0x34), "10:00:00") == 0;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    return ok ? 0 : 1;
}

extern "C" int gamenet_wait_for_local_player_color_index_smoke(void) {
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldSessionRuntime = g_zNetwork_SessionRuntimeInitialized;

    g_zNetwork_LocalPlayerRecord = nullptr;
    const bool noWaitOk = GameNet::WaitForLocalPlayerColorIndex(0) == 0;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.colorIndex = 6;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_SessionRuntimeInitialized = 0;
    const bool colorOk = GameNet::WaitForLocalPlayerColorIndex(1) == 6;

    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_SessionRuntimeInitialized = oldSessionRuntime;
    return noWaitOk && colorOk ? 0 : 1;
}

extern "C" int net_init_from_zrd_smoke(void) {
    char oldDir[MAX_PATH] = {};
    if (GetCurrentDirectoryA(sizeof(oldDir), oldDir) == 0) {
        return 1;
    }

    char tempRoot[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempRoot), tempRoot) == 0) {
        return 2;
    }

    char tempDir[MAX_PATH] = {};
    std::sprintf(tempDir, "%srecoil_gamenet_init", tempRoot);
    CreateDirectoryA(tempDir, nullptr);
    if (SetCurrentDirectoryA(tempDir) == 0) {
        return 3;
    }

    zReader::Node spawn0Values[5] = {};
    MakeGameNetReaderFloatNode(spawn0Values[1], 1.0f);
    MakeGameNetReaderFloatNode(spawn0Values[2], 2.0f);
    MakeGameNetReaderFloatNode(spawn0Values[3], 3.0f);
    MakeGameNetReaderFloatNode(spawn0Values[4], 10.0f);
    zReader::Node spawn0{};
    MakeGameNetReaderArrayNode(spawn0, spawn0Values, 5);

    zReader::Node spawn1Values[5] = {};
    MakeGameNetReaderFloatNode(spawn1Values[1], 4.0f);
    MakeGameNetReaderFloatNode(spawn1Values[2], 5.0f);
    MakeGameNetReaderFloatNode(spawn1Values[3], 6.0f);
    MakeGameNetReaderFloatNode(spawn1Values[4], 90.0f);
    zReader::Node spawn1{};
    MakeGameNetReaderArrayNode(spawn1, spawn1Values, 5);

    zReader::Node spawnListValues[3] = {};
    spawnListValues[1] = spawn0;
    spawnListValues[2] = spawn1;
    zReader::Node spawnList{};
    MakeGameNetReaderArrayNode(spawnList, spawnListValues, 3);

    zReader::Node rootValues[2] = {};
    rootValues[1] = spawnList;
    zReader::Node root{};
    MakeGameNetReaderArrayNode(root, rootValues, 2);

    zArchiveList *const oldMountedList = g_zArchive_MountedList;
    zIndexArchive archive{};
    zZarFileRecord records[1] = {};
    zArchiveListNode archiveNode{};
    zArchiveList archiveList{};
    const GameNetZrdArchiveEntry entries[] = {{"net.zrd", &root}};
    if (!MountGameNetZrdArchive("gamenet_init.zar", entries, 1, archive, records, archiveNode,
                                archiveList)) {
        SetCurrentDirectoryA(oldDir);
        return 4;
    }

    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    zUtil_SaveGameState *const oldSaveHead = g_PlayerSaveStateListHead;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnList = g_GameNetSpawnPointList;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTimerPanel *const oldTimerPanel = g_HudUiMgrTimerPanel;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;
    const int oldHostTimerInitFlag = g_GameNetHostHudTimerInitFlag;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const float oldNextSendTime = g_GameNetPkt06NextSendTimeSec;
    const int oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const int oldRuntimeTimerRaw = g_HudSensorTracker.runtimeTimerSecRaw;
    const int oldMissionId = g_HudSensorTracker.missionId;
    HWND const oldWindow = g_zInput_hWnd;
    const int oldCenterX = g_zInput_MouseClientCenterX;
    const int oldCenterY = g_zInput_MouseClientCenterY;
    const zInput::MouseStateSnapshot oldMouseState = g_zInput_MouseStateSnapshot;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND const hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 20, 30, 160, 120,
                                      nullptr, nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        g_zArchive_MountedList = oldMountedList;
        CloseHandle(static_cast<HANDLE>(archive.hFile));
        std::remove("gamenet_init.zar");
        SetCurrentDirectoryA(oldDir);
        return 5;
    }

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial modalNode = {};
    modalNode.classId = 5;
    modalNode.classData = &objectData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    modalState.modalNode = &modalNode;
    modalData.masterType = 3;

    zNetwork_PlayerRecord localRecord{};
    localRecord.playerKey = 0x1234;
    localRecord.colorIndex = 2;
    std::strcpy(localRecord.playerName, "Local");
    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode localNode{};
    sentinel.next = &localNode;
    sentinel.prev = &localNode;
    localNode.next = &sentinel;
    localNode.prev = &sentinel;
    localNode.playerRecord = &localRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    zNetworkDPlaySessionDescCache session{};
    session.desc.dwMaxPlayers = 8;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    HudUiTimerPanel timer{};
    HudUiPanel *const timerPanel = reinterpret_cast<HudUiPanel *>(&timer);
    timerPanel->ConstructorDefault("", 0, 0);

    g_PlayerSaveStateListHead = nullptr;
    g_GameNetPlayerRowList = 0;
    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;
    g_GameNetSpawnPointList = 0;
    g_GameNetSpawnPointHead = nullptr;
    g_GameNetSpawnPointTail = nullptr;
    g_GameNetSpawnPointCount = 0;
    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_zNetwork_LocalPlayerKey = localRecord.playerKey;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_PlayerRecordList = &playerList;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_HudUiMgrStatsList = &statsList;
    g_HudUiMgrTimerPanel = &timer;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0x41f00000;
    g_HudSensorTracker.missionId = 11;
    g_GameNetHostHudTimerInitFlag = 99;
    g_GameNetPkt06InitialSyncGate = 0;
    g_GameNetPkt06NextSendTimeSec = 7.0f;
    g_HudTimerPanelNetState.timeWarningShown = 1;
    g_HudTimerPanelNetState.oneMinuteWarningShown = 1;
    g_zInput_hWnd = hwnd;
    g_zInput_MouseClientCenterX = 32;
    g_zInput_MouseClientCenterY = 24;

    Net::InitFromZrd();

    GameNetPlayerRow *const row = saveState.netPlayerRow;
    GameNetSpawnPoint *const firstSpawn = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const secondSpawn = firstSpawn != nullptr ? firstSpawn->next : nullptr;
    int spawnFailure = 0;
    if (firstSpawn == nullptr) {
        spawnFailure = 20;
    } else if (secondSpawn == nullptr) {
        spawnFailure = 21;
    } else if (secondSpawn->next != nullptr) {
        spawnFailure = 22;
    } else if (!Vec3Equals(firstSpawn->position, {1.0f, 2.0f, 3.0f})) {
        spawnFailure = 23;
    } else if (!FloatNear(firstSpawn->yawDegrees, 10.0f)) {
        spawnFailure = 24;
    } else if (!Vec3Equals(secondSpawn->position, {4.0f, 5.0f, 6.0f})) {
        spawnFailure = 25;
    } else if (!FloatNear(secondSpawn->yawDegrees, 90.0f)) {
        spawnFailure = 26;
    }
    const bool spawnOk = spawnFailure == 0;
    const bool rowOk = row != nullptr && row->playerKey == 0x1234 &&
                       row->playerColorIndex == 2 && std::strcmp(row->displayName, "Local") == 0 &&
                       row->playerColorPackedRgb == g_GameNetPlayerRowStyleColors_00RRGGBB[2];
    const bool hostTimerOk =
        g_GameNetHostHudTimerInitFlag == 0 && g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
        FloatNear(g_HudTimerPanelNetState.statusBitsResendDeadline, 30.0f) &&
        g_HudTimerPanelNetState.timeWarningShown == 0 &&
        g_HudTimerPanelNetState.oneMinuteWarningShown == 0 &&
        FloatNear(FieldAt<float>(&timer, 0x2a4), 30.0f) &&
        FieldAt<std::int32_t>(&timer, 0x2ac) == -1;
    const bool respawnOk =
        secondSpawn != nullptr && Vec3Equals(playerState.worldPos, secondSpawn->position) &&
        FloatNear(playerState.restartYawRad, 1.5707964f) &&
        saveState.netPlayerRow == row && g_GameNetPkt06InitialSyncGate == 1 &&
        g_GameNetPkt06NextSendTimeSec == 0.0f;

    GameNetSpawnPoint *spawn = g_GameNetSpawnPointHead;
    while (spawn != nullptr) {
        GameNetSpawnPoint *const next = spawn->next;
        ::operator delete(spawn);
        spawn = next;
    }
    ::operator delete(row);

    DeleteObject(timerPanel->hFont);
    timerPanel->hFont = nullptr;
    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    g_GameStateOrMapTable = oldGameState;
    g_PlayerSaveStateListHead = oldSaveHead;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_GameNetSpawnPointList = oldSpawnList;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiMgrTimerPanel = oldTimerPanel;
    g_HudTimerPanelNetState = oldTimerState;
    g_GameNetHostHudTimerInitFlag = oldHostTimerInitFlag;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_GameNetPkt06NextSendTimeSec = oldNextSendTime;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeTimerSecRaw = oldRuntimeTimerRaw;
    g_HudSensorTracker.missionId = oldMissionId;
    g_zInput_hWnd = oldWindow;
    g_zInput_MouseClientCenterX = oldCenterX;
    g_zInput_MouseClientCenterY = oldCenterY;
    g_zInput_MouseStateSnapshot = oldMouseState;
    g_VariantTag_Current = oldVariantTagCurrent;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_zArchive_MountedList = oldMountedList;
    CloseHandle(static_cast<HANDLE>(archive.hFile));
    std::remove("gamenet_init.zar");
    SetCurrentDirectoryA(oldDir);
    RemoveDirectoryA(tempDir);

    if (!spawnOk) {
        return spawnFailure;
    }
    if (!rowOk) {
        return 11;
    }
    if (!hostTimerOk) {
        return 12;
    }
    if (!respawnOk) {
        return 13;
    }
    return 0;
}

extern "C" int net_format_ipv4_address_smoke(void) {
    char text[32];

    Net::FormatIpv4Address(text, 0);
    if (std::strcmp(text, "0.0.0.0") != 0) {
        return 1;
    }

    Net::FormatIpv4Address(text, 0x01020304);
    if (std::strcmp(text, "4.3.2.1") != 0) {
        return 2;
    }

    Net::FormatIpv4Address(text, 0xffffffff);
    if (std::strcmp(text, "255.255.255.255") != 0) {
        return 3;
    }

    Net::FormatIpv4Address(text, 0xc0a80164);
    if (std::strcmp(text, "100.1.168.192") != 0) {
        return 4;
    }

    return 0;
}

extern "C" int gamenet_host_update_session_status_fields_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "mission";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    session.desc.dwUser1 = 1;
    session.desc.dwUser2 = 2;
    session.desc.dwUser3 = 3;
    session.desc.dwUser4 = 4;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 0;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 0 ||
        g_setSessionDescCalls != 0 || session.desc.dwUser1 != 1) {
        return 1;
    }

    g_zNetwork_IsHostFlag = 1;
    if (GameNet::HostUpdateSessionDescStatusFields(10, 13, 12, 11) != 1 ||
        g_setSessionDescCalls != 1 || session.desc.dwUser1 != 10 ||
        session.desc.dwUser2 != 11 || session.desc.dwUser3 != 12 ||
        session.desc.dwUser4 != 13 || session.desc.dwMaxPlayers != 8 ||
        std::strcmp(sessionName, "mission") != 0) {
        return 2;
    }

    g_setSessionDescResult = static_cast<std::int32_t>(0x88770014);
    if (GameNet::HostUpdateSessionDescStatusFields(20, 23, 22, 21) != 0 ||
        session.desc.dwUser1 != 20) {
        return 3;
    }

    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_CurrentSessionDescCache = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    return 0;
}

extern "C" int gamenet_timer_status_packet_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(42.0f, -1.0f);

    g_Time_AccumulatedTimeSec = 12.0f;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    GameNet::SendPkt0C_HudTimerStatusBits(&g_HudTimerPanelNetState);

    const bool sent = g_sendCalls == 1 && g_sendFlags == 1 &&
                      g_sendPacket == &g_NetPkt0C_HudTimerStatusBitsBuf &&
                      g_sendPacketSize == sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetType == 0x0c &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.packetSizeBytes ==
                          sizeof(NetPkt0C_HudTimerStatusBits) &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.header.payloadDword0 == 0x5678 &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.timerSeconds == 42.0f &&
                      g_NetPkt0C_HudTimerStatusBitsBuf.statusBitsPackedHiWord == 1;

    const bool applied = g_HudTimerPanelNetState.timerSeconds == 42.0f &&
                         g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                         g_HudTimerPanelNetState.statusBitsResendDeadline == 42.0f &&
                         FieldAt<float>(&timer, 0x2a4) == 42.0f &&
                         FieldAt<std::int32_t>(&timer, 0x2ac) == -1;

    NetPkt0C_HudTimerStatusBits packet = {};
    packet.timerSeconds = 7.0f;
    packet.statusBitsPackedHiWord = 0;
    GameNet::HandlePkt0C_HudTimerStatusBits(0, &packet);
    const bool directApply = g_HudTimerPanelNetState.timerSeconds == 7.0f &&
                             g_HudTimerPanelNetState.timerDirectionNeg == 0 &&
                             FieldAt<float>(&timer, 0x2a4) == 7.0f &&
                             FieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_zNetwork_pDirectPlay4 = nullptr;
    g_zNetwork_LocalPlayerRecord = nullptr;
    g_zNetwork_LocalPlayerKey = 0;
    g_zNetwork_IsHostFlag = 0;
    g_Time_AccumulatedTimeSec = 0.0f;
    return sent && applied && directApply ? 0 : 1;
}

extern "C" int gamenet_timer_panel_state_packet_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4321;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x8765;
    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(88.0f, -1.0f);

    HudTimerPanelNetState state{};
    state.timerDirectionNeg = 1;
    state.startGateTriggered = 1;
    state.raceFinishCountdownTriggered = 1;
    GameNet::SendPkt0D_HudTimerPanelState(&state);

    const NetPkt0D_HudTimerPanelState *const sentPacket =
        reinterpret_cast<const NetPkt0D_HudTimerPanelState *>(g_sendPacketBytes);
    const bool sent = g_sendCalls == 1 && g_sendFlags == 1 &&
                      g_sendPacketSize == sizeof(NetPkt0D_HudTimerPanelState) &&
                      sentPacket->header.packetType == 0x0d &&
                      sentPacket->header.packetSizeBytes ==
                          sizeof(NetPkt0D_HudTimerPanelState) &&
                      sentPacket->header.payloadDword0 == 0x8765 &&
                      sentPacket->seconds == 88.0f &&
                      sentPacket->hudTimerFlagsPacked == 0x19;

    const bool localApply = g_HudTimerPanelNetState.timerSeconds == 88.0f &&
                            g_HudTimerPanelNetState.timerDirectionNeg == 1 &&
                            g_HudTimerPanelNetState.startGateTriggered == 1 &&
                            g_HudTimerPanelNetState.raceFinishCountdownTriggered == 1 &&
                            FieldAt<float>(&timer, 0x2a4) == 88.0f &&
                            FieldAt<std::int32_t>(&timer, 0x2ac) == -1;

    NetPkt0D_HudTimerPanelState packet{};
    packet.seconds = 12.0f;
    packet.hudTimerFlagsPacked = 0;
    const bool handled = GameNet::HandlePkt0D_HudTimerPanelState(0, &packet) == 1 &&
                         g_HudTimerPanelNetState.timerSeconds == 12.0f &&
                         g_HudTimerPanelNetState.timerDirectionNeg == 0 &&
                         FieldAt<float>(&timer, 0x2a4) == 12.0f &&
                         FieldAt<std::int32_t>(&timer, 0x2ac) == 1;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_HudTimerPanelNetState = oldTimerState;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    return sent && localApply && handled ? 0 : 1;
}

extern "C" int gamenet_send_pkt14_hud_timer_and_flags_sync_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const NetPkt14_HudTimerAndFlagsSync oldPacket = g_NetPkt14_HudTimerAndFlagsSyncBuf;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int result = GameNet::SendPkt14_HudTimerAndFlagsSync(0x12345, 0xaabbccdd,
                                                               77, 0x23456);
    const NetPkt14_HudTimerAndFlagsSync *const sentPacket =
        reinterpret_cast<const NetPkt14_HudTimerAndFlagsSync *>(g_sendPacketBytes);
    const bool ok =
        result == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt14_HudTimerAndFlagsSyncBuf &&
        g_sendPacketSize == sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetType == 0x14 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.packetSizeBytes ==
            sizeof(NetPkt14_HudTimerAndFlagsSync) &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.header.payloadDword0 == 0x55667788 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.eventCode == 0x2345 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.auxParam == 0x3456 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.valueOrTime == 77 &&
        g_NetPkt14_HudTimerAndFlagsSyncBuf.statusFlags == 0xaabbccdd &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->eventCode == 0x2345 && sentPacket->auxParam == 0x3456 &&
        sentPacket->valueOrTime == 77 && sentPacket->statusFlags == 0xaabbccdd;

    g_NetPkt14_HudTimerAndFlagsSyncBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    return ok ? 0 : 1;
}

extern "C" int gamenet_tick_local_player_pkt06_and_timer_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    int *const oldNetworkEnabled = ZOPT_NETWORK_ENABLED;
    zInput_GameStateOrMapTablePartial *const oldGameState = g_GameStateOrMapTable;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const HudTimerPanelNetState oldTimerState = g_HudTimerPanelNetState;
    const int oldInitialGate = g_GameNetPkt06InitialSyncGate;
    const int oldLatch16 = g_GameNetPkt06InputBit16Latch;
    const int oldLatch17 = g_GameNetPkt06InputBit17Latch;
    const float oldNextSend = g_GameNetPkt06NextSendTimeSec;
    const int oldTenSecondArmed = g_GameNetHudTimerTenSecondWarningArmed;
    const int oldPendingReminderArmed = g_GameNetHudTimerPendingSaveReminderArmed;
    const NetPkt06_PlayerStateSnapshot oldPkt06 = g_NetPkt06_PlayerStateSnapshotBuf;

    int networkEnabled = 1;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x11223344;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_IsHostFlag = 1;

    HudUiTimerPanel timer{};
    HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&timer);
    panel->ConstructorDefault("", 0, 0);
    g_HudUiMgrTimerPanel = &timer;
    HudUiTimerPanel::SetSeconds(120.0f, -1.0f);

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    PlayerModalState modalState{};
    PlayerMasterModalData modalData{};
    GameNetPlayerRow row{};
    zVec3 targetA{1.0f, 2.0f, 3.0f};
    zVec3 targetB{4.0f, 5.0f, 6.0f};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    saveState.netPlayerRow = &row;
    modalState.masterModalData = &modalData;
    modalData.masterType = 5;
    row.playerColorIndex = 7;

    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&saveState));
    g_Time_AccumulatedTimeSec = 20.0f;
    g_FrameDeltaTimeSec = 0.25f;
    g_GameNetPkt06NextSendTimeSec = 19.0f;
    g_GameNetPkt06InputBit16Latch = 0;
    g_GameNetPkt06InputBit17Latch = 0;
    g_GameNetPkt06InitialSyncGate = 0;
    g_HudSensorTracker.raceCheckpointMode = 1;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    g_NetPkt06_PlayerStateSnapshotBuf = {};

    playerState.netInputBit16Latch = 1;
    playerState.netInputBit17Latch = 0;
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 400;
    playerState.altGunAimOrigin = {10.0f, 11.0f, 12.0f};
    playerState.storedTargetPos = {20.0f, 21.0f, 22.0f};
    playerState.worldPos = {30.0f, 31.0f, 32.0f};
    playerState.vehicleRotationAngles = {0.1f, 0.2f, 0.3f};
    playerState.statusMeterValue = 88.0f;
    playerState.progressTargetCount = 2;
    playerState.progressTargetSlots[0].targetPos = &targetA;
    playerState.progressTargetSlots[1].targetPos = &targetB;

    g_sendCalls = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int result = GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const NetPkt06_PlayerStateSnapshot *const sentPacket =
        reinterpret_cast<const NetPkt06_PlayerStateSnapshot *>(g_sendPacketBytes);
    const bool packetOk =
        result == 0 && g_sendCalls == 1 && g_sendFlags == 0 &&
        g_sendPacket == &g_NetPkt06_PlayerStateSnapshotBuf.header &&
        g_sendPacketSize == 0x44 + 4 + 2 * sizeof(zVec3) &&
        sentPacket->header.packetType == 0x06 &&
        sentPacket->header.packetSizeBytes == 0x44 + 4 + 2 * sizeof(zVec3) &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->cachedAltSelectionCode == 301 &&
        sentPacket->cachedPrimarySelectionCode == 400 &&
        (sentPacket->packedMasterTypeColorFlags & 0x7ffffu) ==
            (5u | (7u << 8) | 0x10000u | 0x40000u) &&
        Vec3Equals(sentPacket->altGunAimOrigin, {10.0f, 11.0f, 12.0f}) &&
        Vec3Equals(sentPacket->storedTargetPos, {20.0f, 21.0f, 22.0f}) &&
        Vec3Equals(sentPacket->worldPos, {30.0f, 31.0f, 32.0f}) &&
        Vec3Equals(sentPacket->vehicleRotationAngles, {0.1f, 0.2f, 0.3f}) &&
        sentPacket->statusMeterValue == 88.0f && sentPacket->progressTargetCount == 2 &&
        Vec3Equals(sentPacket->progressTargetPoints[0], targetA) &&
        Vec3Equals(sentPacket->progressTargetPoints[1], targetB) &&
        g_GameNetPkt06InputBit16Latch == 0 && g_GameNetPkt06InputBit17Latch == 0 &&
        FloatNear(g_GameNetPkt06NextSendTimeSec, 20.1f);

    HudUiTimerPanel::SetSeconds(9.0f, -1.0f);
    g_Time_AccumulatedTimeSec = 21.0f;
    g_GameNetPkt06NextSendTimeSec = 20.0f;
    g_HudTimerPanelNetState = {};
    g_HudTimerPanelNetState.timerDirectionNeg = 1;
    g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
    playerState.progressTargetCount = 0;
    g_sendCalls = 0;
    GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer(&saveState);
    const bool countdownTriggeredOk = g_HudTimerPanelNetState.startCountdownTriggered == 1;
    const bool countdownTimerOk = FieldAt<float>(&timer, 0x2a4) == 10.25f &&
                                  FieldAt<std::int32_t>(&timer, 0x2ac) == -1;
    const bool countdownSendOk = g_sendCalls >= 2;

    DeleteObject(panel->hFont);
    panel->hFont = nullptr;
    g_HudUiMgrTimerPanel = nullptr;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    ZOPT_NETWORK_ENABLED = oldNetworkEnabled;
    g_GameStateOrMapTable = oldGameState;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudTimerPanelNetState = oldTimerState;
    g_GameNetPkt06InitialSyncGate = oldInitialGate;
    g_GameNetPkt06InputBit16Latch = oldLatch16;
    g_GameNetPkt06InputBit17Latch = oldLatch17;
    g_GameNetPkt06NextSendTimeSec = oldNextSend;
    g_GameNetHudTimerTenSecondWarningArmed = oldTenSecondArmed;
    g_GameNetHudTimerPendingSaveReminderArmed = oldPendingReminderArmed;
    g_NetPkt06_PlayerStateSnapshotBuf = oldPkt06;
    g_Time_AccumulatedTimeSec = 0.0f;
    g_FrameDeltaTimeSec = 0.0f;

    if (!packetOk) {
        return 1;
    }
    if (!countdownTriggeredOk) {
        return 2;
    }
    if (!countdownTimerOk) {
        return 3;
    }
    if (!countdownSendOk) {
        return 4;
    }
    return 0;
}

extern "C" int gamenet_scoreboard_snapshot_packet_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow alpha{};
    alpha.playerKey = 0x101;
    alpha.playerColorPackedRgb = 0x00112233;
    std::strcpy(alpha.displayName, "Alpha");

    GameNetPlayerRow bravo{};
    bravo.playerKey = 0x202;
    bravo.playerColorPackedRgb = 0x00445566;
    std::strcpy(bravo.displayName, "Bravo");
    alpha.next = &bravo;

    g_GameNetPlayerRowHead = &alpha;
    g_GameNetPlayerRowTail = &bravo;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    g_zNetwork_IsHostFlag = 0;

    GameNet::RefreshPlayerListMenu(&alpha);
    GameNet::RefreshPlayerListMenu(&bravo);

    ScoreboardPacket2 packet{};
    packet.header.packetType = 0x09;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.entryCount = 2;
    packet.entries[0].playerKey = alpha.playerKey;
    packet.entries[0].packedScoreAndLapCount = static_cast<std::uint16_t>((3 << 9) | 17);
    packet.entries[1].playerKey = bravo.playerKey;
    packet.entries[1].packedScoreAndLapCount = static_cast<std::uint16_t>((4 << 9) | 22);

    const std::int32_t handleResult = GameNet::HandlePkt09_PlayerScoreboardSnapshot(
        0, reinterpret_cast<NetPkt09_PlayerScoreboardSnapshot *>(&packet));
    const bool applied = handleResult == 1 && alpha.score == 17 && alpha.lapCount == 3 &&
                         bravo.score == 22 && bravo.lapCount == 4 &&
                         triplet.entries.begin[0].playerKey == bravo.playerKey &&
                         triplet.entries.begin[1].playerKey == alpha.playerKey;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    alpha.score = 33;
    alpha.lapCount = 7;
    bravo.score = 44;
    bravo.lapCount = 5;
    GameNet::SendPkt09_PlayerScoreboardSnapshot();

    const ScoreboardPacket2 *const sentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool sent =
        g_sendCalls == 1 && g_sendFlags == 1 && g_sendPacketSize == sizeof(ScoreboardPacket2) &&
        g_sendPacketBytesSize == sizeof(ScoreboardPacket2) &&
        sentPacket->header.packetType == 0x09 &&
        sentPacket->header.packetSizeBytes == sizeof(ScoreboardPacket2) &&
        sentPacket->header.payloadDword0 == 0x5678 && sentPacket->entryCount == 2 &&
        sentPacket->entries[0].playerKey == alpha.playerKey &&
        sentPacket->entries[0].packedScoreAndLapCount ==
            static_cast<std::uint16_t>((7 << 9) | 33) &&
        sentPacket->entries[1].playerKey == bravo.playerKey &&
        sentPacket->entries[1].packedScoreAndLapCount == static_cast<std::uint16_t>((5 << 9) | 44);

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();

    return applied && sent ? 0 : 1;
}

extern "C" int gamenet_lap_progress_packet_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x2222;
    g_zNetwork_IsHostFlag = 0;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    zUtil_PlayerStateStorage playerState{};
    playerState.lapCount = 4;
    playerState.lapTimeSec = 65.0f;
    zUtil_SaveGameState saveState{};
    GameNetPlayerRow localRow{};
    saveState.playerState = &playerState;
    saveState.netPlayerRow = &localRow;

    GameNet::SendPkt0E_PlayerLapProgress(&saveState);
    const NetPkt0E_PlayerLapProgress *const sentPacket =
        reinterpret_cast<const NetPkt0E_PlayerLapProgress *>(g_sendPacketBytes);
    const bool clientSend =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacketSize == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.packetType == 0x0e &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0E_PlayerLapProgress) &&
        sentPacket->header.payloadDword0 == 0x2222 && sentPacket->lapCountPacked == 4 &&
        sentPacket->lapTimeSec == 65.0f && localRow.lapCount == 0;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    GameNetPlayerRow remoteRow{};
    remoteRow.playerKey = 0x3333;
    remoteRow.playerColorPackedRgb = 0x00123456;
    std::strcpy(remoteRow.displayName, "Remote");
    g_GameNetPlayerRowHead = &remoteRow;
    g_GameNetPlayerRowTail = &remoteRow;
    g_GameNetPlayerRowCount = 1;
    g_HudSensorTracker.runtimeGoalValue = 3;
    GameNet::RefreshPlayerListMenu(&remoteRow);

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    NetPkt0E_PlayerLapProgress packet{};
    packet.lapCountPacked = 2;
    packet.lapTimeSec = 44.0f;
    const bool hostHandle = GameNet::HandlePkt0E_PlayerLapProgress(0x3333, &packet) == 1 &&
                            remoteRow.lapCount == 2 && remoteRow.lapTimeSec == 44.0f &&
                            g_sendCalls == 1;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    triplet.DestructorCore();
    return clientSend && hostHandle ? 0 : 1;
}

extern "C" int gamenet_chat_message_packet_smoke(void) {
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;
    HudUiTextStack4 *const oldChatStack = g_HudUiChatMessageStack;

    HudUiChatMessageStack chat{};
    chat.Constructor();
    chat.base.enabled = 1;
    g_HudUiChatMessageStack = &chat;

    NetPkt0B_ChatMessage packet = {};
    packet.messageLength = 5;
    std::memcpy(packet.message, "hello", 5);
    GameNet::HandlePkt0B_ChatMessage(0, &packet);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&chat.lines[0][0]);
    const bool shortMessage = std::strcmp(firstLine->GetLastTextPtr(), "hello") == 0 &&
                              FieldAt<float>(firstLine, 0x10) == 5.0f;

    NetPkt0B_ChatMessage longPacket = {};
    longPacket.messageLength = 0x55;
    for (std::size_t index = 0; index < sizeof(longPacket.message); ++index) {
        longPacket.message[index] = static_cast<char>('A' + (index % 26));
    }

    GameNet::HandlePkt0B_ChatMessage(0, &longPacket);
    const char *const text = firstLine->GetLastTextPtr();
    const bool clamped = std::strlen(text) == sizeof(longPacket.message) &&
                         std::memcmp(text, longPacket.message, sizeof(longPacket.message)) == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x10203040;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x10203040;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::SendPkt0B_ChatMessage("hello");

    const NetPkt0B_ChatMessage *const sentPacket =
        reinterpret_cast<const NetPkt0B_ChatMessage *>(g_sendPacketBytes);
    const bool sentShort = g_sendCalls == 1 && g_sendFlags == 1 &&
                           g_sendPacketSize == 17 && g_sendPacketBytesSize == 17 &&
                           sentPacket->header.packetType == 0x0b &&
                           sentPacket->header.packetSizeBytes == 17 &&
                           sentPacket->header.payloadDword0 == 0x10203040 &&
                           sentPacket->messageLength == 5 &&
                           std::memcmp(sentPacket->message, "hello", 5) == 0 &&
                           g_sendPacketBytes[15] == 0 && g_sendPacketBytes[16] == 0;

    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0xff, sizeof(g_sendPacketBytes));

    GameNet::SendPkt0B_ChatMessage("");

    const NetPkt0B_ChatMessage *const emptyPacket =
        reinterpret_cast<const NetPkt0B_ChatMessage *>(g_sendPacketBytes);
    const bool sentEmpty = g_sendCalls == 1 && g_sendFlags == 1 &&
                           g_sendPacketSize == 12 && g_sendPacketBytesSize == 12 &&
                           emptyPacket->header.packetType == 0x0b &&
                           emptyPacket->header.packetSizeBytes == 12 &&
                           emptyPacket->header.payloadDword0 == 0x10203040 &&
                           emptyPacket->messageLength == 0 &&
                           g_sendPacketBytes[10] == 0 && g_sendPacketBytes[11] == 0;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&chat.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiChatMessageStack = oldChatStack;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    return shortMessage && clamped && sentShort && sentEmpty ? 0 : 1;
}

extern "C" int gamenet_show_player_kill_message_smoke(void) {
    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    GameNetPlayerRow victim{};
    std::strcpy(victim.displayName, "Victim");
    GameNetPlayerRow killer{};
    std::strcpy(killer.displayName, "Killer");
    OptCatalogEntryDef killEntry{};
    killEntry.killVerbString = const_cast<char *>("tagged");

    GameNet::ShowPlayerKillMessage(&victim, &killEntry, &killer);

    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool ok = std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0 &&
                    FieldAt<float>(firstLine, 0x10) == 2.0f;

    for (int index = 0; index < 4; ++index) {
        HudUiPanel *const panel = reinterpret_cast<HudUiPanel *>(&top.lines[index][0]);
        DeleteObject(panel->hFont);
        panel->hFont = nullptr;
    }

    g_HudUiTopMessageStack = nullptr;
    return ok ? 0 : 1;
}

extern "C" int gamenet_player_kill_event_packet_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    const std::int32_t oldRaceMode = g_HudSensorTracker.raceCheckpointMode;
    const std::int32_t oldGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const std::int32_t oldOneLapShown = g_GameNetOneLapLeftMessageShown;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldOptCatalogEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldOptCatalogEntryTable = g_OptCatalog_EntryTable;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const std::int32_t oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const std::int32_t oldIsHost = g_zNetwork_IsHostFlag;
    const std::int32_t oldTcpIpAsync = g_zNetwork_TcpIpAsyncSendEnabled;

    HudUiTopMessageStack top{};
    top.Constructor();
    top.base.enabled = 1;
    g_HudUiTopMessageStack = &top;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    OptCatalogEntryDef killEntry{};
    killEntry.keyName = const_cast<char *>("test_weapon");
    killEntry.ordinalIndex = 3;
    killEntry.killVerbString = const_cast<char *>("tagged");
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &killEntry;

    GameNetPlayerRow killer{};
    killer.playerKey = 0x11;
    killer.lapCount = 1;
    killer.playerColorPackedRgb = 0x00112233;
    std::strcpy(killer.displayName, "Killer");

    GameNetPlayerRow victim{};
    victim.playerKey = 0x22;
    victim.score = 4;
    victim.lapCount = 2;
    victim.playerColorPackedRgb = 0x00445566;
    std::strcpy(victim.displayName, "Victim");
    killer.next = &victim;

    g_GameNetPlayerRowHead = &killer;
    g_GameNetPlayerRowTail = &victim;
    g_GameNetPlayerRowCount = 2;
    g_HudSensorTracker.raceCheckpointMode = 0;
    g_HudSensorTracker.runtimeGoalValue = 999;
    g_GameNetOneLapLeftMessageShown = 0;
    GameNet::RefreshPlayerListMenu(&killer);
    GameNet::RefreshPlayerListMenu(&victim);

    NetPkt08_PlayerKillEvent packet{};
    packet.killMethodOrOptCatalogEntryId = 3;
    packet.targetPlayerKey = victim.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;

    const std::int32_t nonHostResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    HudUiPanel *const firstLine = reinterpret_cast<HudUiPanel *>(&top.lines[0][0]);
    const bool nonHost = nonHostResult == 1 && victim.score == 4 && g_sendCalls == 0 &&
                         std::strcmp(firstLine->GetLastTextPtr(), "Victim tagged Killer") == 0;

    packet.targetPlayerKey = 0x7777;
    const bool missingRow = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet) == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x4444;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x5678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    packet.targetPlayerKey = victim.playerKey;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t hostResult = GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const hostSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool host = hostResult == 1 && victim.score == 5 && g_sendCalls == 1 &&
                      g_sendPacketSize == sizeof(ScoreboardPacket2) &&
                      hostSentPacket->entries[1].playerKey == victim.playerKey &&
                      hostSentPacket->entries[1].packedScoreAndLapCount ==
                          static_cast<std::uint16_t>((victim.lapCount << 9) | 5);

    packet.targetPlayerKey = killer.playerKey;
    killer.score = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const std::int32_t suicideResult =
        GameNet::HandlePkt08_PlayerKillEvent(killer.playerKey, &packet);
    const ScoreboardPacket2 *const suicideSentPacket =
        reinterpret_cast<const ScoreboardPacket2 *>(g_sendPacketBytes);
    const bool suicide = suicideResult == 1 && killer.score == 0 && g_sendCalls == 1 &&
                         suicideSentPacket->entries[0].playerKey == killer.playerKey &&
                         suicideSentPacket->entries[0].packedScoreAndLapCount ==
                             static_cast<std::uint16_t>(killer.lapCount << 9);

    zUtil_SaveGameState saveState{};
    saveState.netPlayerRow = &victim;
    g_zNetwork_LocalPlayerKey = killer.playerKey;
    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(&saveState, 3);
    const NetPkt08_PlayerKillEvent *const sentKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool explicitSaveStateSend =
        g_sendCalls == 1 && g_sendPacketSize == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.packetType == 0x08 &&
        sentKillPacket->header.packetSizeBytes == sizeof(NetPkt08_PlayerKillEvent) &&
        sentKillPacket->header.payloadDword0 == killer.playerKey &&
        sentKillPacket->killMethodOrOptCatalogEntryId == 3 &&
        sentKillPacket->targetPlayerKey == victim.playerKey;

    g_GameStateOrMapTable = reinterpret_cast<zInput_GameStateOrMapTablePartial *>(&saveState);
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendPkt08_PlayerKillEvent(nullptr, 3);
    const NetPkt08_PlayerKillEvent *const fallbackKillPacket =
        reinterpret_cast<const NetPkt08_PlayerKillEvent *>(g_sendPacketBytes);
    const bool fallbackSaveStateSend =
        g_sendCalls == 1 && fallbackKillPacket->header.payloadDword0 == killer.playerKey &&
        fallbackKillPacket->targetPlayerKey == victim.playerKey;

    g_HudUiTopMessageStack = oldTopStack;
    g_HudUiMgrStatsList = oldStatsList;
    g_HudSensorTracker.raceCheckpointMode = oldRaceMode;
    g_HudSensorTracker.runtimeGoalValue = oldGoalValue;
    g_GameNetOneLapLeftMessageShown = oldOneLapShown;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldOptCatalogEntryCount;
    g_OptCatalog_EntryTable = oldOptCatalogEntryTable;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldTcpIpAsync;
    DeleteTopMessageStackFonts(top);
    triplet.DestructorCore();

    return nonHost && missingRow && host && suicide && explicitSaveStateSend &&
                   fallbackSaveStateSend
               ? 0
               : 1;
}

extern "C" int gamenet_reassign_player_colors_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;

    zNetwork_PlayerRecord firstRecord{};
    firstRecord.playerKey = 0x1111;
    firstRecord.colorIndex = 8;
    zNetwork_PlayerRecord secondRecord{};
    secondRecord.playerKey = 0x2222;
    secondRecord.colorIndex = 2;

    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode firstNode{};
    zNetworkPlayerRecordListNode secondNode{};
    sentinel.next = &firstNode;
    sentinel.prev = &secondNode;
    firstNode.next = &secondNode;
    firstNode.prev = &sentinel;
    firstNode.playerRecord = &firstRecord;
    secondNode.next = &sentinel;
    secondNode.prev = &firstNode;
    secondNode.playerRecord = &secondRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 2;
    g_zNetwork_PlayerRecordList = &playerList;

    zNetworkDPlaySessionDescCache session{};
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_CurrentSessionDescCache = &session;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial firstObject{};
    zClass_NodePartial firstObjectNode{};
    firstObjectNode.classId = 5;
    firstObjectNode.classData = &firstObject;
    PlayerModalState firstModal{};
    firstModal.modalNode = &firstObjectNode;
    GameNetPlayerSaveState firstSave{};
    firstSave.primaryModalState = &firstModal;

    zClass_Object3DDataPartial secondObject{};
    zClass_NodePartial secondObjectNode{};
    secondObjectNode.classId = 5;
    secondObjectNode.classData = &secondObject;
    PlayerModalState secondModal{};
    secondModal.modalNode = &secondObjectNode;
    GameNetPlayerSaveState secondSave{};
    secondSave.primaryModalState = &secondModal;

    GameNetPlayerRow firstRow{};
    firstRow.playerKey = firstRecord.playerKey;
    firstRow.saveState = &firstSave;
    std::strcpy(firstRow.displayName, "First");
    GameNetPlayerRow secondRow{};
    secondRow.playerKey = secondRecord.playerKey;
    secondRow.saveState = &secondSave;
    std::strcpy(secondRow.displayName, "Second");
    firstRow.next = &secondRow;
    g_GameNetPlayerRowHead = &firstRow;
    g_GameNetPlayerRowTail = &secondRow;
    g_GameNetPlayerRowCount = 2;

    GameNet::RefreshPlayerListMenu(&firstRow);
    GameNet::RefreshPlayerListMenu(&secondRow);
    const std::int32_t result = GameNet::ReassignPlayerColorsAndRefreshRows(0, nullptr);

    const HudUiScoreboardEntry *firstEntry = nullptr;
    const HudUiScoreboardEntry *secondEntry = nullptr;
    for (HudUiScoreboardEntry *entry = triplet.entries.begin; entry != triplet.entries.end;
         ++entry) {
        if (entry->playerKey == firstRow.playerKey) {
            firstEntry = entry;
        }
        if (entry->playerKey == secondRow.playerKey) {
            secondEntry = entry;
        }
    }

    const bool firstOk = firstRow.playerColorIndex == 8 &&
                         firstRow.playerColorPackedRgb == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x14c) == 0x000040ff &&
                         FieldAt<std::uint32_t>(&firstRow.hudWidget, 0x150) == 0x000040ff &&
                         FieldAt<std::int32_t>(&firstRow.hudWidget, 0x270) == 1 &&
                         firstEntry != nullptr && firstEntry->playerColorPackedRgb == 0x000040ff &&
                         firstObject.color.red == 1.0f && firstObject.color.green == 1.0f &&
                         firstObject.color.blue == 0.0f && firstObject.colorAlpha == 0.2f;

    const bool secondOk =
        secondRow.playerColorIndex == 2 && secondRow.playerColorPackedRgb == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x14c) == 0x0000ff00 &&
        FieldAt<std::uint32_t>(&secondRow.hudWidget, 0x150) == 0x0000ff00 &&
        FieldAt<std::int32_t>(&secondRow.hudWidget, 0x270) == 1 && secondEntry != nullptr &&
        secondEntry->playerColorPackedRgb == 0x0000ff00 && secondObject.color.red == 0.0f &&
        secondObject.color.green == 1.0f && secondObject.color.blue == 0.0f &&
        secondObject.colorAlpha == 0.2f;

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    triplet.DestructorCore();

    return result == 1 && firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_player_row_apply_color_tint_smoke(void) {
    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerModalState modalState{};
    modalState.modalNode = &objectNode;
    GameNetPlayerSaveState saveState{};
    saveState.primaryModalState = &modalState;
    GameNetPlayerRow row{};
    row.playerColorIndex = 8;
    row.saveState = &saveState;

    row.ApplyPlayerColorTint();
    return objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
                   objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
                   (objectData.flags & 4) != 0
               ? 0
               : 1;
}

extern "C" int gamenet_apply_pkt06_player_state_snapshot_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const int oldFrameTick = g_zVideo_FrameTick;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData{};
    modalData.masterType = 5;
    PlayerModalState modalState{};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState{};
    GameNetPlayerSaveState saveState{};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow row{};
    row.playerKey = 0x2468;
    row.playerColorIndex = 1;
    row.saveState = &saveState;
    std::strcpy(row.displayName, "Pkt06");
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;
    GameNet::RefreshPlayerListMenu(&row);

    zVec3 staleTargets[10]{};
    for (int index = 0; index < 10; ++index) {
        playerState.progressTargetRuntimeSlots[index].targetPos = &staleTargets[index];
    }
    playerState.cachedAltSelectionCode = 301;
    playerState.cachedPrimarySelectionCode = 201;
    playerState.netLastUpdateFrameTick = oldFrameTick - 1;
    g_zVideo_FrameTick = oldFrameTick + 17;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.cachedAltSelectionCode = 301;
    packet.cachedPrimarySelectionCode = 201;
    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x10000u;
    packet.storedTargetPos = {1.0f, 2.0f, 3.0f};
    packet.worldPos = {4.0f, 5.0f, 6.0f};
    packet.vehicleRotationAngles = {0.25f, 0.5f, 0.75f};
    packet.statusMeterValue = 77.0f;

    const int clearResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(&row, &packet);
    bool clearedSlots = true;
    for (int index = 0; index < 10; ++index) {
        clearedSlots = clearedSlots && playerState.progressTargetRuntimeSlots[index].targetPos == nullptr;
    }
    const bool firstOk =
        clearResult == 1 && playerState.netUpdateReceived == 1 &&
        row.playerColorIndex == 8 && row.playerColorPackedRgb == 0x000040ff &&
        FieldAt<std::uint32_t>(&row.hudWidget, 0x14c) == 0x000040ff &&
        FieldAt<std::uint32_t>(&row.hudWidget, 0x150) == 0x000040ff &&
        FieldAt<std::int32_t>(&row.hudWidget, 0x270) == 1 &&
        objectData.color.red == 1.0f && objectData.color.green == 1.0f &&
        objectData.color.blue == 0.0f && objectData.colorAlpha == 0.2f &&
        Vec3Equals(playerState.netReceivedPos, packet.worldPos) &&
        Vec3Equals(playerState.netReceivedAngles, packet.vehicleRotationAngles) &&
        Vec3Equals(playerState.storedTargetPos, packet.storedTargetPos) &&
        playerState.netInputBit16Latch == 1 && playerState.netInputBit17Latch == 0 &&
        playerState.netLastUpdateFrameTick == g_zVideo_FrameTick &&
        FloatNear(playerState.statusMeterValue, 77.0f) &&
        playerState.progressTargetCount == 0 && clearedSlots;

    packet.packedMasterTypeColorFlags = 5u | (8u << 8) | 0x20000u | 0x40000u;
    packet.statusMeterValue = 88.0f;
    packet.progressTargetCount = 2;
    packet.progressTargetPoints[0] = {10.0f, 11.0f, 12.0f};
    packet.progressTargetPoints[1] = {20.0f, 21.0f, 22.0f};

    const int targetResult = GameNet::ApplyPkt06_PlayerStateSnapshotToRow(&row, &packet);
    const bool secondOk =
        targetResult == 1 && playerState.netInputBit16Latch == 1 &&
        playerState.netInputBit17Latch == 1 && FloatNear(playerState.statusMeterValue, 88.0f) &&
        playerState.progressTargetCount == 2 &&
        playerState.progressTargetRuntimeSlots[0].targetPos ==
            &playerState.progressTargetPointStorage[0] &&
        playerState.progressTargetRuntimeSlots[1].targetPos ==
            &playerState.progressTargetPointStorage[1] &&
        Vec3Equals(playerState.progressTargetPointStorage[0], packet.progressTargetPoints[0]) &&
        Vec3Equals(playerState.progressTargetPointStorage[1], packet.progressTargetPoints[1]);

    g_HudUiMgrStatsList = oldStatsList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_zVideo_FrameTick = oldFrameTick;
    triplet.DestructorCore();

    return firstOk && secondOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt06_player_state_snapshot_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const int oldInitialSyncGate = g_GameNetPkt06InitialSyncGate;
    const int oldFrameTick = g_zVideo_FrameTick;

    const int nullResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, nullptr);

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial objectNode{};
    objectNode.classId = 5;
    objectNode.classData = &objectData;

    PlayerMasterModalData modalData{};
    modalData.masterType = 5;
    PlayerModalState modalState{};
    modalState.masterModalData = &modalData;
    modalState.modalNode = &objectNode;

    zUtil_PlayerStateStorage playerState{};
    GameNetPlayerSaveState saveState{};
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;

    GameNetPlayerRow row{};
    row.playerKey = 0x2468;
    row.playerColorIndex = 1;
    row.saveState = &saveState;
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    NetPkt06_PlayerStateSnapshot ignoredPacket{};
    ignoredPacket.header.packetType = 5;
    ignoredPacket.header.payloadDword0 = row.playerKey;
    g_GameNetPkt06InitialSyncGate = 1;
    const int ignoredResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, &ignoredPacket);
    const bool ignoredOk =
        ignoredResult == 0 && g_GameNetPkt06InitialSyncGate == 0 && playerState.netUpdateReceived == 0;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = row.playerKey;
    packet.packedMasterTypeColorFlags = 5u | (1u << 8);
    packet.worldPos = {8.0f, 9.0f, 10.0f};
    packet.vehicleRotationAngles = {0.125f, 0.25f, 0.5f};
    g_zVideo_FrameTick = oldFrameTick + 3;

    const int handledResult = GameNet::HandlePkt06_PlayerStateSnapshot(0x1111, &packet);
    const bool handledOk = handledResult == 0 && playerState.netUpdateReceived == 1 &&
                           row.playerColorIndex == 1 &&
                           Vec3Equals(playerState.netReceivedPos, packet.worldPos) &&
                           Vec3Equals(playerState.netReceivedAngles, packet.vehicleRotationAngles);

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_GameNetPkt06InitialSyncGate = oldInitialSyncGate;
    g_zVideo_FrameTick = oldFrameTick;

    return nullResult == -1 && ignoredOk && handledOk ? 0 : 1;
}

extern "C" int gamenet_spawn_remote_player_missing_template_smoke(void) {
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    zNetworkPlayerRecordList *const oldPlayerRecordList = g_zNetwork_PlayerRecordList;
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;

    zNetwork_PlayerRecord playerRecord{};
    playerRecord.playerKey = 0x1111;
    std::strcpy(playerRecord.playerName, "Remote");
    zNetworkPlayerRecordListNode sentinel{};
    zNetworkPlayerRecordListNode playerNode{};
    sentinel.next = &playerNode;
    sentinel.prev = &playerNode;
    playerNode.next = &sentinel;
    playerNode.prev = &sentinel;
    playerNode.playerRecord = &playerRecord;
    zNetworkPlayerRecordList playerList{};
    playerList.sentinelNode = &sentinel;
    playerList.count = 1;
    g_zNetwork_PlayerRecordList = &playerList;

    HudUiTopMessageStack topStack{};
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    g_GameNetPlayerRowHead = nullptr;
    g_GameNetPlayerRowTail = nullptr;
    g_GameNetPlayerRowCount = 0;

    NetPkt06_PlayerStateSnapshot packet{};
    packet.header.packetType = 6;
    packet.header.payloadDword0 = 0x2222;

    const int result = GameNet::SpawnRemotePlayerFromPkt06_PlayerStateSnapshot(
        static_cast<int>(playerRecord.playerKey), &packet);
    const bool ok = result == 0 && g_GameNetPlayerRowHead == nullptr &&
                    g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0;

    g_HudUiTopMessageStack = oldTopStack;
    g_zNetwork_PlayerRecordList = oldPlayerRecordList;
    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;

    return ok ? 0 : 1;
}

extern "C" int gamenet_handle_pkt07_alt_gun_dispatch_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    void *const oldPendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
    void *const oldPendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;

    NetPkt07_AltGunDispatch missingPacket{};
    missingPacket.header.payloadDword0 = 0x4040;
    const int missingResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &missingPacket);

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    saveState.playerState = &playerState;

    PlayerGunFireController originalController{};
    PlayerGunFireController targetController{};
    playerState.activeAltGunController = &originalController;
    playerState.altGunFireHeldFlag = 1;
    playerState.worldPos = {2.0f, 3.0f, 4.0f};
    playerState.steerBasisRaw = {0.0f, 0.0f, 1.0f};

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 707;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    targetController.optCatalogEntry = &entry;
    playerState.altWeaponBanks[4].controllerA = targetController;

    GameNetPlayerRow row{};
    row.playerKey = 0x3030;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    NetPkt07_AltGunDispatch packet{};
    packet.header.payloadDword0 = row.playerKey;
    packet.weaponId = 707;
    packet.dispatchFlags = 0x1234;
    packet.targetPos = {10.0f, 11.0f, 12.0f};
    g_OptCatalogPendingSpawnTargetCountPtr = (void *)(0x11112222);
    g_OptCatalogPendingSpawnTargetListPtr = (void *)(0x33334444);

    const int handledResult = GameNet::HandlePkt07_AltGunDispatch(0x1111, &packet);
    const bool handledOk =
        handledResult == 1 && playerState.altGunDispatchFlags == 0 &&
        playerState.activeAltGunController == &originalController &&
        Vec3Equals(playerState.storedTargetPos, packet.targetPos) &&
        Vec3Equals(playerState.altFireOrigin, {2.0f, 4.0f, 4.0f}) &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalogPendingSpawnTargetCountPtr = oldPendingSpawnTargetCountPtr;
    g_OptCatalogPendingSpawnTargetListPtr = oldPendingSpawnTargetListPtr;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;

    return missingResult == 0 && handledOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt07_alt_gun_dispatch_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_PlayerStateStorage playerState{};
    playerState.storedTargetPos = {9.0f, 8.0f, 7.0f};
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1234;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x55667788;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0}, 0, 0,
                                    0, {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    GameNet::SendPkt07_AltGunDispatch(static_cast<short>(0x8123), 0x01000001u);

    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool ok =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt07_AltGunDispatchBuf.header &&
        g_sendPacketSize == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.packetType == 7 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt07_AltGunDispatch) &&
        sentPacket->header.payloadDword0 == 0x55667788 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0x01000001u &&
        Vec3Equals(sentPacket->targetPos, playerState.storedTargetPos);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    return ok ? 0 : 1;
}

extern "C" int optcatalog_alt_gun_dispatch_alloc_runtime_gate_smoke(void) {
    const NetPkt07_AltGunDispatch oldPacket = g_NetPkt07_AltGunDispatchBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    OptCatalogEntryDef passEntry{};
    passEntry.ordinalIndex = 0;
    void *passSlot = reinterpret_cast<void *>(0x11223344u);
    const int passZeroResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    passEntry.ordinalIndex = 1;
    const int passOneResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&passEntry, &passSlot);
    const bool passOk = passZeroResult == 1 && passOneResult == 1 &&
                        passSlot == reinterpret_cast<void *>(0x11223344u);

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 0x8123;
    void *nullSlot = nullptr;
    const bool nullOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &nullSlot) == 0 &&
        nullSlot == nullptr;

    zUtil_PlayerStateStorage localPlayerState{};
    localPlayerState.storedTargetPos = {1.0f, 2.0f, 3.0f};
    zUtil_SaveGameState localSaveState{};
    localSaveState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localSaveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x2468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x13572468;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt07_AltGunDispatchBuf = {{0x07, sizeof(NetPkt07_AltGunDispatch), 0}, 0, 0,
                                    0, {0.0f, 0.0f, 0.0f}};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    void *localSlot = &localSaveState;
    const int localResult =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &localSlot);
    const NetPkt07_AltGunDispatch *const sentPacket =
        reinterpret_cast<const NetPkt07_AltGunDispatch *>(g_sendPacketBytes);
    const bool localOk =
        localResult == 1 && localSlot == reinterpret_cast<void *>(0x01000000u) &&
        g_sendCalls == 1 && sentPacket->header.payloadDword0 == 0x13572468 &&
        sentPacket->weaponId == static_cast<short>(0x8123) &&
        sentPacket->dispatchFlags == 0 &&
        Vec3Equals(sentPacket->targetPos, localPlayerState.storedTargetPos);

    zUtil_PlayerStateStorage remotePlayerState{};
    remotePlayerState.altGunDispatchFlags = 0x01000001;
    zUtil_SaveGameState remoteSaveState{};
    remoteSaveState.playerState = &remotePlayerState;
    void *remoteSlot = &remoteSaveState;
    const bool remoteRejectedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 0 &&
        remoteSlot == &remoteSaveState;

    remotePlayerState.altGunDispatchFlags = 0x02000012;
    remoteSlot = &remoteSaveState;
    const bool remoteAcceptedOk =
        OptCatalog::AltGunDispatchAllocRuntimeGateCallback(&entry, &remoteSlot) == 1 &&
        remoteSlot == reinterpret_cast<void *>(0x02000012u);

    g_NetPkt07_AltGunDispatchBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!passOk) {
        return 1;
    }
    if (!nullOk) {
        return 2;
    }
    if (!localOk) {
        return 3;
    }
    if (!remoteRejectedOk) {
        return 4;
    }
    return remoteAcceptedOk ? 0 : 5;
}

extern "C" int gamenet_alt_gun_dispatch_no_op_callback_smoke(void) {
    OptCatalogEntryDef entry{};
    void *saveStateSlot = nullptr;
    return GameNet::AltGunDispatchNoOpCallback(&entry, &saveStateSlot) == 1 ? 0 : 1;
}

extern "C" int optcatalog_handle_pkt0a_remove_runtime_relay_smoke(void) {
    GameNetPlayerRow *const oldHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldTail = g_GameNetPlayerRowTail;
    const std::uint32_t oldCount = g_GameNetPlayerRowCount;
    const std::int32_t oldEntryCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldEntryTable = g_OptCatalog_EntryTable;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;

    NetPkt0A_RemoveRuntimeRelay packet{};
    packet.ownerPlayerKey = 0x9090;
    const int missingResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);

    zClass_NodePartial ownerRoot{};
    zUtil_PlayerStateStorage playerState{};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;

    GameNetPlayerRow row{};
    row.playerKey = packet.ownerPlayerKey;
    row.saveState = (GameNetPlayerSaveState *)(&saveState);
    g_GameNetPlayerRowHead = &row;
    g_GameNetPlayerRowTail = &row;
    g_GameNetPlayerRowCount = 1;

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 303;
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable = &entry;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    packet.optCatalogEntryId = 303;
    packet.pointOrVec3 = {0.0f, 0.0f, 0.0f};
    const int handledZeroResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool zeroOk = handledZeroResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    packet.pointOrVec3 = {1.0f, 0.0f, 0.0f};
    const int handledPointResult = OptCatalog::HandlePkt0A_RemoveRuntimeRelay(0x1111, &packet);
    const bool pointOk = handledPointResult == 1 && g_OptCatalogProcessRuntimeRelayEnabled == 1;

    g_GameNetPlayerRowHead = oldHead;
    g_GameNetPlayerRowTail = oldTail;
    g_GameNetPlayerRowCount = oldCount;
    g_OptCatalog_EntryCount = oldEntryCount;
    g_OptCatalog_EntryTable = oldEntryTable;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;

    return missingResult == 0 && zeroOk && pointOk ? 0 : 1;
}

extern "C" int optcatalog_send_pkt0a_remove_runtime_relay_smoke(void) {
    const NetPkt0A_RemoveRuntimeRelay oldPacket = g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf;
    const int oldRelayEnabled = g_OptCatalogProcessRuntimeRelayEnabled;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    OptCatalogEntryDef entry{};
    entry.ordinalIndex = 0x4567;

    g_OptCatalogProcessRuntimeRelayEnabled = 0;
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, nullptr);
    const bool disabledOk = g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x1111;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_OptCatalogProcessRuntimeRelayEnabled = 1;

    zClass_NodePartial ownerNode{};
    g_sendCalls = 0;
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const bool missingContextOk = g_sendCalls == 0;

    GameNetPlayerRow ownerRow{};
    ownerRow.playerKey = 0x2468;
    zUtil_SaveGameState ownerSaveState{};
    ownerSaveState.netPlayerRow = &ownerRow;
    HudUiMgrSensorTrackNode trackNode{};
    trackNode.payload = &ownerSaveState;
    ownerNode.callbackContext = (zClass_NodePartial *)&trackNode;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf =
        {{0x0a, sizeof(NetPkt0A_RemoveRuntimeRelay), 0},
         0,
         0,
         {9.0f, 9.0f, 9.0f},
         0};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    zVec3 point{4.0f, 5.0f, 6.0f};
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, &point, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const pointPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool pointOk =
        g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0A_RemoveRuntimeRelay) &&
        pointPacket->header.payloadDword0 == 0x12345678 &&
        pointPacket->optCatalogEntryId == static_cast<short>(0x4567) &&
        Vec3Equals(pointPacket->pointOrVec3, point) &&
        pointPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = {9.0f, 9.0f, 9.0f};
    g_sendCalls = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    OptCatalog::SendPkt0A_RemoveRuntimeRelay(&entry, nullptr, &ownerNode);
    const NetPkt0A_RemoveRuntimeRelay *const zeroPacket =
        reinterpret_cast<const NetPkt0A_RemoveRuntimeRelay *>(g_sendPacketBytes);
    const bool zeroOk =
        g_sendCalls == 1 && zeroPacket->pointOrVec3.x == 0.0f &&
        zeroPacket->pointOrVec3.y == 0.0f && zeroPacket->pointOrVec3.z == 0.0f &&
        zeroPacket->ownerPlayerKey == ownerRow.playerKey;

    g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf = oldPacket;
    g_OptCatalogProcessRuntimeRelayEnabled = oldRelayEnabled;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!disabledOk) {
        return 1;
    }
    if (!missingContextOk) {
        return 2;
    }
    if (!pointOk) {
        return 3;
    }
    return zeroOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt10_qsand_feature_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;

    zDEClient_QSandEventTemplate eventTemplate{};
    eventTemplate.radius = 12.5f;
    eventTemplate.center = {7.0f, 8.0f, 9.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x5555;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt10_QSandEventSendBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12u, 0,
                                    {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt10_QSandFeature(&eventTemplate);
    const NetPkt10_QSandEvent *const sentPacket =
        reinterpret_cast<const NetPkt10_QSandEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.packetType == 0x10 &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == 0x12345678 &&
        sentPacket->eventFlags == (0x12u | 0x80u) &&
        Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt10_QSandEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt10_qsand_event_smoke(void) {
    const NetPkt10_QSandEvent oldPacket = g_NetPkt10_QSandEventRelayBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zDEClient_NetRelayCallback const oldRelayCallback = g_zDEClientQSandNetRelayCallback;
    const zDEClient_QSandEventTemplate oldDefaults = g_zDEClient_QuickSandEventTemplateDefaults;

    zClass_NodePartial ownerRoot{};
    zClass_NodePartial otherRoot{};
    zUtil_PlayerStateStorage playerState{};
    playerState.rootNode = &ownerRoot;
    zUtil_SaveGameState saveState{};
    saveState.playerState = &playerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&saveState;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_zDEClientQSandNetRelayCallback = QSandRelayCallbackFake;
    g_zDEClient_QuickSandEventTemplateDefaults = {};
    g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 4;

    zDEClient_QSandEventTemplate negativeEvent{};
    negativeEvent.radius = -2.25f;
    const int negativeResult = GameNet::SendPkt10_QSandEvent(&negativeEvent);
    const bool negativeOk = negativeResult == 1 && FloatNear(negativeEvent.radius, 2.25f);

    g_NetPkt10_QSandEventRelayBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate otherOwnerEvent{};
    otherOwnerEvent.radius = 5.0f;
    otherOwnerEvent.center = {1.0f, 2.0f, 3.0f};
    otherOwnerEvent.damageOwnerNode = &otherRoot;
    const int otherOwnerResult = GameNet::SendPkt10_QSandEvent(&otherOwnerEvent);
    const bool otherOwnerOk = otherOwnerResult == 0 && g_sendCalls == 0 &&
                              g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 == 0 &&
                              g_NetPkt10_QSandEventRelayBuf.eventFlags == 0x12345678u;

    zDEClient_QSandEventTemplate nonHostEvent{};
    nonHostEvent.radius = 6.5f;
    nonHostEvent.center = {4.0f, 5.0f, 6.0f};
    nonHostEvent.damageOwnerNode = &ownerRoot;
    g_zNetwork_IsHostFlag = 0;
    g_NetPkt10_QSandEventRelayBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x12345678u, 0,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    const int nonHostResult = GameNet::SendPkt10_QSandEvent(&nonHostEvent);
    const NetPkt10_QSandEvent *const sentPacket =
        reinterpret_cast<const NetPkt10_QSandEvent *>(g_sendPacketBytes);
    const bool nonHostOk =
        nonHostResult == 0 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt10_QSandEventRelayBuf.header &&
        g_sendPacketSize == sizeof(NetPkt10_QSandEvent) &&
        sentPacket->header.payloadDword0 == localPlayer.playerKey &&
        sentPacket->eventFlags == 0x12340000u && Vec3Equals(sentPacket->center, nonHostEvent.center) &&
        FloatNear(sentPacket->radius, 6.5f);

    g_zNetwork_IsHostFlag = 1;
    g_qsandRelayCallbackCount = 0;
    g_qsandRelayCallbackResult = 0;
    g_sendCalls = 0;
    zDEClient_QSandEventTemplate hostEvent = nonHostEvent;
    hostEvent.radius = 7.75f;
    g_NetPkt10_QSandEventRelayBuf = {{0x10, sizeof(NetPkt10_QSandEvent), 0}, 0x87654321u, 0,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    const int hostResult = GameNet::SendPkt10_QSandEvent(&hostEvent);
    const bool hostOk =
        hostResult == 0 && g_qsandRelayCallbackCount == 1 && g_sendCalls == 0 &&
        g_NetPkt10_QSandEventRelayBuf.header.payloadDword0 == localPlayer.playerKey &&
        g_NetPkt10_QSandEventRelayBuf.eventFlags == 0x87650000u &&
        Vec3Equals(g_NetPkt10_QSandEventRelayBuf.center, hostEvent.center) &&
        FloatNear(g_NetPkt10_QSandEventRelayBuf.radius, 7.75f);

    g_NetPkt10_QSandEventRelayBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    g_zDEClientQSandNetRelayCallback = oldRelayCallback;
    g_zDEClient_QuickSandEventTemplateDefaults = oldDefaults;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;

    if (!negativeOk) {
        return 1;
    }
    if (!otherOwnerOk) {
        return 2;
    }
    if (!nonHostOk) {
        return 3;
    }
    return hostOk ? 0 : 4;
}

extern "C" int gamenet_host_send_pkt0f_crater_feature_smoke(void) {
    const NetPkt0F_CraterEvent oldPacket = g_NetPkt0F_CraterEventSendBuf;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zModel_MaterialSlot *const oldMatlPool = g_zModel_MatlPool;
    const int oldMatlCapacity = g_zModel_MatlPoolCapacity;
    const int oldMatlInUse = g_zModel_MatlPoolInUseCount;

    zModel_MaterialSlot materialSlots[4]{};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 4;
    g_zModel_MatlPoolInUseCount = 4;

    zDEClient_CraterEventTemplate eventTemplate{};
    eventTemplate.craterMaterialSlot = &materialSlots[2];
    eventTemplate.radius = 6.25f;
    eventTemplate.center = {3.0f, 4.0f, 5.0f};

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    const int nonHostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const bool nonHostOk = nonHostResult == 0 && g_sendCalls == 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x7777;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x23456789;
    g_zNetwork_IsHostFlag = 1;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_NetPkt0F_CraterEventSendBuf = {{0x0f, sizeof(NetPkt0F_CraterEvent), 0}, 0x21u, -1,
                                     {0.0f, 0.0f, 0.0f}, 0.0f};
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacket = nullptr;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    const int hostResult = GameNet::HostSendPkt0F_CraterFeature(&eventTemplate);
    const NetPkt0F_CraterEvent *const sentPacket =
        reinterpret_cast<const NetPkt0F_CraterEvent *>(g_sendPacketBytes);
    const bool hostOk =
        hostResult == 1 && g_sendCalls == 1 && g_sendFlags == 1 &&
        g_sendPacket == &g_NetPkt0F_CraterEventSendBuf.header &&
        g_sendPacketSize == sizeof(NetPkt0F_CraterEvent) &&
        g_sendPacketBytesSize == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.packetType == 0x0f &&
        sentPacket->header.packetSizeBytes == sizeof(NetPkt0F_CraterEvent) &&
        sentPacket->header.payloadDword0 == 0x23456789 &&
        sentPacket->eventFlags == (0x21u | 0x80u) && sentPacket->craterTypeId == 2 &&
        Vec3Equals(sentPacket->center, eventTemplate.center) &&
        FloatNear(sentPacket->radius, eventTemplate.radius);

    g_NetPkt0F_CraterEventSendBuf = oldPacket;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zModel_MatlPool = oldMatlPool;
    g_zModel_MatlPoolCapacity = oldMatlCapacity;
    g_zModel_MatlPoolInUseCount = oldMatlInUse;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_send_pkt13_effect_anim_activation_record_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x12345678;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = 0x12345678;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    g_sendCalls = 0;
    g_sendFlags = 0;
    g_sendPacketSize = 0;
    g_sendPacketBytesSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));

    zEffectAnimActivationRecord record{};
    std::memset(&record, 0xab, sizeof(record));
    record.commandType = 2;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);

    const zNetworkPacketHeader *const header =
        reinterpret_cast<const zNetworkPacketHeader *>(g_sendPacketBytes);
    const bool sentOk = g_sendCalls == 1 && g_sendFlags == 1 &&
                        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x48 &&
                        header->packetType == 0x13 &&
                        header->packetSizeBytes == sizeof(zNetworkPacketHeader) + 0x48 &&
                        header->payloadDword0 == 0x12345678 &&
                        std::memcmp(g_sendPacketBytes + sizeof(zNetworkPacketHeader), &record,
                                    0x48) == 0;

    g_GameNetSuppressPkt13ActivationEcho = 1;
    g_sendCalls = 0;
    GameNet::SendPkt13_EffectAnimActivationRecord(&record);
    const bool suppressOk = g_sendCalls == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return sentOk && suppressOk ? 0 : 1;
}

extern "C" int gamenet_handle_pkt13_effect_anim_activation_record_smoke(void) {
    struct Packet13 {
        zNetworkPacketHeader header;
        zEffectAnimActivationRecord record;
    };

    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;
    const int oldSuppressEcho = g_GameNetSuppressPkt13ActivationEcho;

    Packet13 packet = {};
    packet.header.packetType = 0x13;
    packet.header.packetSizeBytes = sizeof(Packet13);
    packet.record.commandType = 2;
    std::strcpy(packet.record.animName, "missing_pkt13_activation");
    packet.record.nodeToken = 77;

    g_zEffectAnim_ActivationRecordTable = nullptr;
    g_zEffectAnim_ActivationRecordCount = 0;
    g_GameNetSuppressPkt13ActivationEcho = 0;
    const int missingResult = GameNet::HandlePkt13_EffectAnimActivationRecord(0, &packet.header);
    const bool missingOk = missingResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = &packet.record;
    g_zEffectAnim_ActivationRecordCount = 1;
    const int duplicateResult = GameNet::HandlePkt13_EffectAnimActivationRecord(0, &packet.header);
    const bool duplicateOk = duplicateResult == 1 && g_GameNetSuppressPkt13ActivationEcho == 0;

    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;
    g_GameNetSuppressPkt13ActivationEcho = oldSuppressEcho;

    return missingOk && duplicateOk ? 0 : 1;
}

extern "C" int gamenet_send_all_pkt13_effect_anim_activation_records_smoke(void) {
    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetwork_PlayerRecord *const oldLocalPlayer = g_zNetwork_LocalPlayerRecord;
    const int oldLocalPlayerKey = g_zNetwork_LocalPlayerKey;
    const int oldIsHost = g_zNetwork_IsHostFlag;
    const int oldAsyncSend = g_zNetwork_TcpIpAsyncSendEnabled;
    zEffectAnimActivationRecord *const oldRecordTable = g_zEffectAnim_ActivationRecordTable;
    const int oldRecordCount = g_zEffectAnim_ActivationRecordCount;

    zNetwork_PlayerRecord localPlayer{};
    localPlayer.playerKey = 0x13572468;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_LocalPlayerRecord = &localPlayer;
    g_zNetwork_LocalPlayerKey = localPlayer.playerKey;
    g_zNetwork_TcpIpAsyncSendEnabled = 0;

    zEffectAnimActivationRecord records[2] = {};
    std::memset(&records[0], 0x11, sizeof(records[0]));
    std::memset(&records[1], 0x22, sizeof(records[1]));
    records[0].commandType = 1;
    records[1].commandType = 3;
    g_zEffectAnim_ActivationRecordTable = records;
    g_zEffectAnim_ActivationRecordCount = 2;

    g_zNetwork_IsHostFlag = 0;
    g_sendCalls = 0;
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const bool nonHostOk = g_sendCalls == 0;

    g_zNetwork_IsHostFlag = 1;
    g_sendCalls = 0;
    g_sendPacketSize = 0;
    std::memset(g_sendPacketBytes, 0, sizeof(g_sendPacketBytes));
    GameNet::SendAllPkt13_EffectAnimActivationRecords();
    const zNetworkPacketHeader *const header =
        reinterpret_cast<const zNetworkPacketHeader *>(g_sendPacketBytes);
    const bool hostOk = g_sendCalls == 2 &&
                        g_sendPacketSize == sizeof(zNetworkPacketHeader) + 0x4c &&
                        header->packetType == 0x13 &&
                        header->payloadDword0 == localPlayer.playerKey &&
                        std::memcmp(g_sendPacketBytes + sizeof(zNetworkPacketHeader), &records[1],
                                    0x4c) == 0;

    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_LocalPlayerRecord = oldLocalPlayer;
    g_zNetwork_LocalPlayerKey = oldLocalPlayerKey;
    g_zNetwork_IsHostFlag = oldIsHost;
    g_zNetwork_TcpIpAsyncSendEnabled = oldAsyncSend;
    g_zEffectAnim_ActivationRecordTable = oldRecordTable;
    g_zEffectAnim_ActivationRecordCount = oldRecordCount;

    return nonHostOk && hostOk ? 0 : 1;
}

extern "C" int gamenet_player_row_destroy_embedded_panel_smoke(void) {
    GameNetPlayerRow row{};
    row.hudWidget.vtbl = &g_HudUiPanel_FTable;

    row.DestroyEmbeddedPanel();
    return row.hudWidget.vtbl == &g_HudUiCommon_FTable && row.hudWidget.textPick == nullptr ? 0 : 1;
}

extern "C" int gamenet_reset_remote_players_and_spawn_lists_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldSpawnList = g_GameNetSpawnPointList;
    GameNetSpawnPoint *const oldSpawnHead = g_GameNetSpawnPointHead;
    GameNetSpawnPoint *const oldSpawnTail = g_GameNetSpawnPointTail;
    const unsigned int oldSpawnCount = g_GameNetSpawnPointCount;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.base.ConstructorDefault();
    g_HudUiTopMessageStack = &topStack;

    GameNetPlayerRow *const firstRow = new GameNetPlayerRow{};
    GameNetPlayerRow *const secondRow = new GameNetPlayerRow{};
    GameNetSpawnPoint *const firstSpawn = new GameNetSpawnPoint{};
    GameNetSpawnPoint *const secondSpawn = new GameNetSpawnPoint{};

    firstRow->playerKey = 0x1201;
    firstRow->playerColorPackedRgb = 0x00112233;
    std::strcpy(firstRow->displayName, "First");
    firstRow->hudWidget.vtbl = &g_HudUiPanel_FTable;
    firstRow->next = secondRow;

    secondRow->playerKey = 0x1202;
    secondRow->playerColorPackedRgb = 0x00445566;
    std::strcpy(secondRow->displayName, "Second");
    secondRow->hudWidget.vtbl = &g_HudUiPanel_FTable;

    firstSpawn->next = secondSpawn;

    g_GameNetPlayerRowList = 1;
    g_GameNetPlayerRowHead = firstRow;
    g_GameNetPlayerRowTail = secondRow;
    g_GameNetPlayerRowCount = 2;
    g_GameNetSpawnPointList = 1;
    g_GameNetSpawnPointHead = firstSpawn;
    g_GameNetSpawnPointTail = secondSpawn;
    g_GameNetSpawnPointCount = 2;

    triplet.AddEntry(firstRow);
    triplet.AddEntry(secondRow);
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&firstRow->hudWidget));
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&secondRow->hudWidget));

    GameNet::ResetRemotePlayersAndSpawnLists();

    const bool listsCleared =
        g_GameNetPlayerRowList == 0 && g_GameNetPlayerRowHead == nullptr &&
        g_GameNetPlayerRowTail == nullptr && g_GameNetPlayerRowCount == 0 &&
        g_GameNetSpawnPointList == 0 && g_GameNetSpawnPointHead == nullptr &&
        g_GameNetSpawnPointTail == nullptr && g_GameNetSpawnPointCount == 0;
    const bool hudCleared =
        topStack.base.childHead == nullptr && topStack.base.childTail == nullptr &&
        triplet.entries.begin != nullptr && triplet.entries.end == triplet.entries.begin;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetSpawnPointList = oldSpawnList;
    g_GameNetSpawnPointHead = oldSpawnHead;
    g_GameNetSpawnPointTail = oldSpawnTail;
    g_GameNetSpawnPointCount = oldSpawnCount;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    triplet.DestructorCore();

    return listsCleared && hudCleared ? 0 : 1;
}

extern "C" int gamenet_handle_pkt14_hud_timer_and_flags_sync_smoke(void) {
    const RecoilApp oldApp = g_RecoilApp;
    const int oldRuntimeGoalValue = g_HudSensorTracker.runtimeGoalValue;
    const int oldRuntimeTimerSecRaw = g_HudSensorTracker.runtimeTimerSecRaw;
    const int oldMissionId = g_HudSensorTracker.missionId;
    const int oldMissionFlags = g_HudSensorTracker.missionFlags;
    const int oldAllowMaps = g_GameNetStatus_AllowMaps;
    const int oldNameTags = g_GameNetStatus_NameTags;
    const int oldHandlersRegistered = g_GameNet_HandlersRegistered;
    zNetworkDispatchHandlerListNode *const oldSentinel = g_zNetwork_DispatchHandlerListSentinel;
    const int oldHandlerCount = g_zNetwork_DispatchHandlerListCount;
    zNetwork_DPlay4 *const oldDPlay = g_zNetwork_pDirectPlay4;
    zNetworkDPlaySessionDescCache *const oldSession = g_zNetwork_CurrentSessionDescCache;
    const int oldIsHost = g_zNetwork_IsHostFlag;

    std::memset(&g_RecoilApp, 0, sizeof(g_RecoilApp));
    CZRecoilFrame mainWnd{};
    mainWnd.m_useArchiveBanks = 77;
    g_RecoilApp.m_pMainWnd = static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&mainWnd));
    g_RecoilApp.m_currentStateIndex = -1;
    g_RecoilApp.m_introFmvState.base.vftable =
        static_cast<RecoilPtr32>(reinterpret_cast<std::uintptr_t>(&g_pkt14StateVtable));
    g_RecoilApp.m_missionFmvState.m_missionId = 99;

    g_HudSensorTracker.runtimeGoalValue = 0;
    g_HudSensorTracker.runtimeTimerSecRaw = 0;
    g_HudSensorTracker.missionId = 0;
    g_HudSensorTracker.missionFlags = 0;
    g_GameNetStatus_AllowMaps = 0;
    g_GameNetStatus_NameTags = 0;
    g_GameNet_HandlersRegistered = 1;
    g_pkt14StateEnterCount = 0;

    zNetworkDispatchHandlerListNode sentinel{};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    g_zNetwork_DispatchHandlerListSentinel = &sentinel;
    g_zNetwork_DispatchHandlerListCount = 0;

    zNetwork_DPlay4 dplay{&kDPlayVtable};
    zNetworkDPlaySessionDescCache session{};
    char sessionName[0x5c] = "pkt14";
    session.desc.lpszSessionNameA = sessionName;
    session.desc.dwMaxPlayers = 8;
    g_zNetwork_pDirectPlay4 = &dplay;
    g_zNetwork_CurrentSessionDescCache = &session;
    g_zNetwork_IsHostFlag = 1;
    g_setSessionDescCalls = 0;
    g_setSessionDescResult = 0;

    NetPkt14_HudTimerAndFlagsSync packet{};
    packet.header.packetType = 0x14;
    packet.header.packetSizeBytes = sizeof(packet);
    packet.eventCode = 4;
    packet.auxParam = 12;
    packet.valueOrTime = 3;
    packet.statusFlags = 3;

    const int result = GameNet::HandlePkt14_HudTimerAndFlagsSync(0x2222, &packet);

    union TimerSecondsBits {
        float seconds;
        int raw;
    } expectedTimer = {180.0f};
    RecoilApp_StateQueue &queue = g_RecoilApp.m_stateQueue;
    bool queuedIntro = false;
    if (queue.m_itemCount == 1) {
        const RecoilPtr32 slotValue = queue.m_writeBlock.m_cursor - 4;
        RecoilPtr32 *const slot =
            reinterpret_cast<RecoilPtr32 *>(static_cast<std::uintptr_t>(slotValue));
        RecoilApp_StateQueueItem *const item =
            reinterpret_cast<RecoilApp_StateQueueItem *>(static_cast<std::uintptr_t>(*slot));
        queuedIntro = item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent &&
                      item->m_stateObj == static_cast<RecoilPtr32>(
                                             reinterpret_cast<std::uintptr_t>(
                                                 &g_RecoilApp.m_introFmvState.base)) &&
                      item->m_param == 0;
        CleanupSingleQueuedItem(queue);
    }

    int failure = 0;
    if (result != 1) {
        failure = 1;
    } else if (g_GameNet_HandlersRegistered != 0) {
        failure = 2;
    } else if (g_HudSensorTracker.runtimeTimerSecRaw != expectedTimer.raw ||
               g_HudSensorTracker.runtimeGoalValue != 12) {
        failure = 3;
    } else if (g_HudSensorTracker.missionId != 10 || g_HudSensorTracker.missionFlags != 77) {
        failure = 4;
    } else if (g_GameNetStatus_AllowMaps != 1 || g_GameNetStatus_NameTags != 1) {
        failure = 5;
    } else if (g_RecoilApp.m_missionFmvState.m_missionId != 0 ||
               g_pkt14StateEnterCount != 1 || !queuedIntro) {
        failure = 6;
    } else if (g_setSessionDescCalls != 1 || session.desc.dwUser1 != 4 ||
               session.desc.dwUser4 != 12 || session.desc.dwUser3 != 3 ||
               session.desc.dwUser2 != 3) {
        failure = 7;
    } else if (session.desc.dwMaxPlayers != 8 || std::strcmp(sessionName, "pkt14") != 0) {
        failure = 8;
    }

    g_RecoilApp = oldApp;
    g_HudSensorTracker.runtimeGoalValue = oldRuntimeGoalValue;
    g_HudSensorTracker.runtimeTimerSecRaw = oldRuntimeTimerSecRaw;
    g_HudSensorTracker.missionId = oldMissionId;
    g_HudSensorTracker.missionFlags = oldMissionFlags;
    g_GameNetStatus_AllowMaps = oldAllowMaps;
    g_GameNetStatus_NameTags = oldNameTags;
    g_GameNet_HandlersRegistered = oldHandlersRegistered;
    g_zNetwork_DispatchHandlerListSentinel = oldSentinel;
    g_zNetwork_DispatchHandlerListCount = oldHandlerCount;
    g_zNetwork_pDirectPlay4 = oldDPlay;
    g_zNetwork_CurrentSessionDescCache = oldSession;
    g_zNetwork_IsHostFlag = oldIsHost;

    return failure;
}

extern "C" int gamenet_handle_pkt03_remove_remote_player_smoke(void) {
    HudUiStatsListElement *const oldStatsList = g_HudUiMgrStatsList;
    HudUiTextStack4 *const oldTopStack = g_HudUiTopMessageStack;
    const unsigned int oldRowList = g_GameNetPlayerRowList;
    GameNetPlayerRow *const oldRowHead = g_GameNetPlayerRowHead;
    GameNetPlayerRow *const oldRowTail = g_GameNetPlayerRowTail;
    const unsigned int oldRowCount = g_GameNetPlayerRowCount;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;

    HudUiTriplet triplet{};
    triplet.Constructor();
    HudUiStatsListElement statsList{};
    statsList.triplet = &triplet;
    g_HudUiMgrStatsList = &statsList;

    HudUiTopMessageStack topStack{};
    topStack.base.ConstructorDefault();
    topStack.base.enabled = 0;
    g_HudUiTopMessageStack = &topStack;

    HudUiPanel_FTable panelTable = {};
    panelTable.slots[0x60 / 4] = MethodAddress(&TestRemoteHudPanelOps::SetVisible);
    g_remoteHudSetVisibleCount = 0;
    g_remoteHudLastVisible = 7;

    GameNetPlayerRow first{};
    first.playerKey = 0x3101;
    std::strcpy(first.displayName, "First");
    first.hudWidget.vtbl = &panelTable;

    GameNetPlayerRow *const removed = new GameNetPlayerRow{};
    removed->playerKey = 0x3102;
    removed->playerColorPackedRgb = 0x00123456;
    std::strcpy(removed->displayName, "Removed");
    removed->hudWidget.vtbl = &panelTable;
    first.next = removed;

    zUtil_SaveGameState saveState{};
    zUtil_PlayerStateStorage playerState{};
    saveState.playerState = &playerState;
    removed->saveState = (GameNetPlayerSaveState *)&saveState;
    playerState.lifecycleState = 3;
    playerState.cameraTransitionTimer = 0;
    playerState.activeAltGunController = &playerState.altWeaponBanks[2].controllerA;

    zClass_NodePartial rootNode{};
    playerState.rootNode = &rootNode;
    zClass_NodePartial runtimeWorld{};
    runtimeWorld.classId = 3;
    zClass_NodeFreeListSlot projectile{};
    zClass_Object3DDataPartial projectileData{};
    projectile.node.classId = 5;
    projectile.node.classData = &projectileData;
    zClass_NodePartial *worldChildren[1] = {&projectile.node};
    runtimeWorld.listB = worldChildren;
    runtimeWorld.listCountB = 1;

    OptCatalogEntryDef mineEntry{};
    OptCatalogRuntimeInstanceStorage mineRuntime{};
    mineRuntime.ownerNode = &rootNode;
    mineRuntime.projectileNode = &projectile.node;
    mineRuntime.lifetime = 0.0f;
    mineEntry.activeRuntimeListHead = &mineRuntime;
    playerState.altWeaponBanks[4].controllerA.optCatalogEntry = &mineEntry;

    OptCatalogRuntimeInstanceStorage freeSentinel{};
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;

    g_GameNetPlayerRowList = 1;
    g_GameNetPlayerRowHead = &first;
    g_GameNetPlayerRowTail = removed;
    g_GameNetPlayerRowCount = 2;

    triplet.AddEntry(&first);
    triplet.AddEntry(removed);
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&first.hudWidget));
    topStack.base.AddChild(reinterpret_cast<HudUiElement *>(&removed->hudWidget));

    const int result = GameNet::HandlePkt03_RemoveRemotePlayer(removed->playerKey, nullptr);

    const bool playerStateOk =
        result == 0 && playerState.cameraTransitionTimer == 1 && playerState.lifecycleState == 4;
    const bool mineOk = mineEntry.activeRuntimeListHead == nullptr &&
                        g_OptCatalogFreeRuntimeInstanceList == &mineRuntime &&
                        mineRuntime.next == &freeSentinel;
    const bool rowListOk = g_GameNetPlayerRowHead == &first && g_GameNetPlayerRowTail == &first &&
                           g_GameNetPlayerRowCount == 1 && first.next == nullptr;
    const bool hudOk = g_remoteHudSetVisibleCount == 1 && g_remoteHudLastVisible == 0 &&
                       topStack.base.childHead == reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       topStack.base.childTail == reinterpret_cast<HudUiElement *>(&first.hudWidget) &&
                       triplet.entries.begin != nullptr && triplet.entries.end == triplet.entries.begin + 1 &&
                       triplet.entries.begin[0].playerKey == first.playerKey;

    g_HudUiMgrStatsList = oldStatsList;
    g_HudUiTopMessageStack = oldTopStack;
    g_GameNetPlayerRowList = oldRowList;
    g_GameNetPlayerRowHead = oldRowHead;
    g_GameNetPlayerRowTail = oldRowTail;
    g_GameNetPlayerRowCount = oldRowCount;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    triplet.DestructorCore();

    if (!playerStateOk) {
        return 1;
    }
    if (!mineOk) {
        return 2;
    }
    if (!rowListOk) {
        return 3;
    }
    return hudOk ? 0 : 4;
}
