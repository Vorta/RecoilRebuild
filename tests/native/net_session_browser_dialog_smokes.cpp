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
void *TestObjectVtable(void *object)
{
    return *(void **)object;
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

} // namespace
