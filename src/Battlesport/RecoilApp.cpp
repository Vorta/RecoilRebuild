#include "Battlesport/RecoilApp.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetExitPanel.h"
#include "Battlesport/RecoilVersion.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"
#include "pickup.h"
#include "zImage.h"

#include <windows.h>

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 0x0061
#endif

#ifdef FormatMessage
#undef FormatMessage
#endif

#include <direct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class CWinApp {
  public:
    CWinApp(const char *appName);
    virtual ~CWinApp();
    BOOL RECOIL_THISCALL Enable3dControls();
    virtual int ExitInstance();
};

BOOL RECOIL_THISCALL CWinApp::Enable3dControls() {
    return TRUE;
}

class CWnd {
  public:
    static CWnd *RECOIL_STDCALL FromHandle(HWND hwnd);
    BOOL ShowWindow(int showCommand);

    unsigned char unknown_00[0x20];
    HWND m_hWnd;
};

class AFX_MODULE_STATE {
  public:
    void *m_pCurrentWinApp;
    void *unknown_04;
    HINSTANCE m_hCurrentInstanceHandle;
};

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits *RECOIL_THISCALL Constructor();
    static void RECOIL_CDECL QueuePush();
};

AFX_MODULE_STATE *RECOIL_STDCALL AfxGetModuleState();
BOOL RECOIL_STDCALL AfxRegisterClass(WNDCLASSA *wndClass);
HINSTANCE RECOIL_STDCALL AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

namespace {
template <typename Function, typename Method> Function RecoilMethodFunction(Method method)
{
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(Function));
    Function function = 0;
    memcpy(&function, &method, sizeof(method));
    return function;
}

template <typename Method> unsigned int RecoilMethodAddress(Method method)
{
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

struct HudUiSaveLoadBackButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiSaveLoadDialogVtable {
    unsigned int slots[3];
};

struct HudUiSaveLoadDialogUpdateDispatch {
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
};

struct RecoilStateSaveLoadDialogVirtual {
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual RecoilStateSaveLoadDialogVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

struct RecoilStateSaveLoadTransition_Vtbl {
    RecoilFn32 slots[10];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateSaveLoadTransition_Vtbl) ==
                     sizeof(RecoilApp_IState_Vtbl));

enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

enum zVideoSoftwareModeHotkeyState {
    ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED = 0,
    ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED = 1,
};

enum zVideoClearScreenBufferState {
    ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED = 1,
};

const char k_SaveGameNameAllowedChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIKJKLMNOPQRSTUVWXYZ0123456789_ \x1b\r\x08\x7f\x02\x06";
RECOIL_STATIC_ASSERT(sizeof(k_SaveGameNameAllowedChars) == 0x48);

zOpt_ViewRectSection *ViewRectFromPtr(RecoilPtr32 ptr) {
    return (zOpt_ViewRectSection *)((unsigned int)(ptr));
}

LPCSTR IntResource(unsigned int value) {
    return (LPCSTR)(value);
}

RECOIL_FORCEINLINE void ExtendPlayStateTransitionTimer(float seconds) {
    if (g_RecoilApp.m_transitionFadeTimer150 > 0.0) {
        g_RecoilApp.m_transitionFadeTimer150 += seconds;
        return;
    }

    g_RecoilApp.m_transitionFadeTimer150 = seconds;
    zOpt::SetMuteSoundOption(1);
}

void RunGrandPrizeBlurAction() {
    zFMV_ActionBlur blurAction;
    blurAction.Constructor(12, 1);

    zFMV_Action *const action = &blurAction;
    action->vftable->Begin(action, 0.0);
    while (action->vftable->Update(action, 0.0) != 0) {
    }
    action->vftable->End(action);

    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    RecoilStateCredits::QueuePush();

    blurAction.vftable = &g_zFMV_ActionBase_Vtable;
}

RecoilStateSaveLoadTransition_Vtbl g_RecoilStateSaveLoadTransition_Vtbl = {0};

struct RecoilStateSaveLoadTransitionBaseVtableGuard {
    RecoilStateSaveLoadTransition *self;

    ~RecoilStateSaveLoadTransitionBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};

void AppendSaveLoadEntry(HudUiSaveLoadEntries *entries, const HudUiSaveLoadEntry &entry)
{
    const int size = entries->begin != 0 ? (int)(entries->end - entries->begin) : 0;
    const int capacity =
        entries->begin != 0 ? (int)(entries->capacityEnd - entries->begin) : 0;

    if (size >= capacity) {
        const int growth = size > 0 ? size : 1;
        const int newCapacity = size + growth;
        HudUiSaveLoadEntry *const newBegin = (HudUiSaveLoadEntry *)(
            ::operator new(sizeof(HudUiSaveLoadEntry) * newCapacity));

        for (int i = 0; i < size; ++i) {
            newBegin[i] = entries->begin[i];
        }

        ::operator delete(entries->begin);
        entries->begin = newBegin;
        entries->end = newBegin + size;
        entries->capacityEnd = newBegin + newCapacity;
    }

    *entries->end = entry;
    ++entries->end;
}

int SaveLoadEntryCount(const HudUiSaveLoadDialog *dialog)
{
    return dialog->fileEntries.begin != 0
               ? (int)(dialog->fileEntries.end - dialog->fileEntries.begin)
               : 0;
}

HudUiSaveLoadListItemVtable MakeHudUiSaveLoadListItemVtable()
{
    HudUiSaveLoadListItemVtable table = {0};
    table.Invalidate =
        RecoilMethodFunction<HudUiSaveLoadInvalidateFn>(&HudUiPanel::Invalidate);
    table.OnActivate =
        RecoilMethodFunction<HudUiSaveLoadOnActivateFn>(&HudUiSaveLoadListItem::OnActivate);
    table.SetVisible =
        RecoilMethodFunction<HudUiSaveLoadSetVisibleFn>(&HudUiElement::SetVisible);
    table.SetTextFmt =
        RecoilMethodFunction<HudUiSaveLoadSetTextFmtFn>(&HudUiPanel::SetTextFmt);
    return table;
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiSaveLoadWidgetPostLoadNoOp()
{
}

HudUiWidget_FTable MakeHudUiSaveLoadButtonFTable(unsigned int activateCallback)
{
    HudUiWidget_FTable table = {0};
    table.slots[0] = RecoilMethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = RecoilMethodAddress(&HudUiWidget::Draw);
    table.slots[3] = RecoilMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = RecoilMethodAddress(&HudUiElement::SetX);
    table.slots[5] = RecoilMethodAddress(&HudUiElement::SetY);
    table.slots[6] = RecoilMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = RecoilMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = RecoilMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = activateCallback;
    table.slots[15] = RecoilMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = RecoilMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = RecoilMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = RecoilMethodAddress(&HudUiElement::GetX);
    table.slots[26] = RecoilMethodAddress(&HudUiElement::GetY);
    table.slots[30] = RecoilMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = RecoilMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = RecoilMethodAddress(&HudUiSaveLoadWidgetPostLoadNoOp);
    return table;
}

HudUiNumericTextInput_Base_FTable MakeHudUiSaveLoadGameNameInputFTable()
{
    HudUiNumericTextInput_Base_FTable table = g_HudUiNumericTextInput_Base_FTable;
    table.slots[12] = RecoilMethodAddress(&HudUiSaveLoadGameNameInput::OnActivate);
    table.slots[33] = RecoilMethodAddress(&HudUiSaveLoadGameNameInput::OnRawKeyboardEvent);
    return table;
}

HudUiSaveLoadDialogVtable MakeHudUiSaveLoadDialogVtable()
{
    HudUiSaveLoadDialogVtable table = {0};
    table.slots[0] = RecoilMethodAddress(&HudUiBackground::Update);
    table.slots[1] = RecoilMethodAddress(&HudUiBackground::SetEnabled);
    return table;
}

} // namespace

RecoilApp g_RecoilApp;
RecoilStateSaveLoadTransition g_RecoilStateSaveLoadTransition;
const HudUiSaveLoadListItemVtable g_HudUiSaveLoadListItem_Vtbl =
    MakeHudUiSaveLoadListItemVtable();
const HudUiWidget_FTable g_HudUiSaveLoad_DeleteButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(RecoilMethodAddress(&HudUiSaveLoadDeleteButton::OnActivate));
const HudUiWidget_FTable g_HudUiSaveLoad_NextButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(RecoilMethodAddress(&HudUiSaveLoadNextButton::OnActivate));
const HudUiWidget_FTable g_HudUiSaveLoad_PrevButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(RecoilMethodAddress(&HudUiSaveLoadPrevButton::OnActivate));
const HudUiWidget_FTable g_HudUiSaveLoad_BackButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(RecoilMethodAddress(&HudUiSaveLoadBackButton::OnActivate));
const HudUiWidget_FTable g_HudUiSaveGame_PrimaryActionButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(
        RecoilMethodAddress(&HudUiSaveGamePrimaryActionButton::OnActivate));
const HudUiWidget_FTable g_HudUiLoadGame_PrimaryActionButton_Vtbl =
    MakeHudUiSaveLoadButtonFTable(
        RecoilMethodAddress(&HudUiLoadGamePrimaryActionButton::OnActivate));
const HudUiNumericTextInput_Base_FTable g_HudUiSaveLoadGameNameInput_Vtbl =
    MakeHudUiSaveLoadGameNameInputFTable();
const HudUiSaveLoadDialogVtable g_HudUiSaveLoadDialog_Vtbl =
    MakeHudUiSaveLoadDialogVtable();
const HudUiSaveLoadDialogVtable g_HudUiSaveGameDialog_Vtbl =
    MakeHudUiSaveLoadDialogVtable();
const HudUiSaveLoadDialogVtable g_HudUiLoadGameDialog_Vtbl =
    MakeHudUiSaveLoadDialogVtable();

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" {
const char *g_RecoilApp_WndClassNamePtr = "RecoilClass";
int g_RecoilApp_WindowClassRegistered = 0;
int g_RecoilApp_AttractFmvReloadMode = 1;
}

// Reimplements 0x435a30: RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE void RECOIL_CDECL
RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit()
{
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x435a40: RecoilStateSaveLoadTransition::StaticInit
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE RecoilStateSaveLoadTransition *RECOIL_CDECL
RecoilStateSaveLoadTransition::StaticInit()
{
    return g_RecoilStateSaveLoadTransition.Constructor();
}

// Reimplements 0x435a50: RecoilStateSaveLoadTransition::RegisterAtExit
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateSaveLoadTransition::RegisterAtExit()
{
    atexit(AtExitDestructor);
}

// Reimplements 0x435a60: RecoilStateSaveLoadTransition::AtExitDestructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE void RECOIL_CDECL RecoilStateSaveLoadTransition::AtExitDestructor()
{
    g_RecoilStateSaveLoadTransition.Destructor();
}

// Reimplements 0x435c80: RecoilStateSaveLoadTransition::Constructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RecoilStateSaveLoadTransition *RECOIL_THISCALL
RecoilStateSaveLoadTransition::Constructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateSaveLoadTransition_Vtbl;
    m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    m_dialog = 0;
    return this;
}

// Reimplements 0x435ca0: RecoilStateSaveLoadTransition::ScalarDeletingDestructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE RecoilStateSaveLoadTransition *RECOIL_THISCALL
RecoilStateSaveLoadTransition::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x435cc0: RecoilStateSaveLoadTransition::Destructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilStateSaveLoadTransition::Destructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateSaveLoadTransition_Vtbl;
    RecoilStateSaveLoadTransitionBaseVtableGuard baseVtableOnExit = {this};

    RecoilStateSaveLoadDialogVirtual *dialog =
        (RecoilStateSaveLoadDialogVirtual *)m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
        m_dialog = 0;
    }
}

// Reimplements 0x434660: HudUiSaveLoadEntry::IsNewerThan
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
HudUiSaveLoadEntry::IsNewerThan(const HudUiSaveLoadEntry *other) const
{
    return CompareFileTime(&ftLastWriteTime, &other->ftLastWriteTime) > 0 ? 1 : 0;
}

// Reimplements 0x434920: HudUiSaveLoadListItem::Constructor
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE HudUiSaveLoadListItem *RECOIL_THISCALL
HudUiSaveLoadListItem::Constructor()
{
    ((HudUiPanel *)(this))->ConstructorDefault(0, 0, 0);
    vftable = &g_HudUiSaveLoadListItem_Vtbl;
    layoutY = 32767;
    layoutX = -1;
    return this;
}

// Reimplements 0x435a10: HudUiSaveLoadListItem::OnActivate
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
void RECOIL_THISCALL HudUiSaveLoadListItem::OnActivate()
{
    if (parent != 0) {
        parent->SetSelectedEntryIndex(layoutX);
    }
}

// Reimplements 0x434fb0: HudUiSaveLoadDialog::DeleteSaveFile
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::DeleteSaveFile(int confirmDelete)
{
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    int shouldDelete = 1;
    if (confirmDelete != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(titleText, zLoc::GetMessageString(138));
        strcpy(messageText, zLoc::GetMessageString(139));
        shouldDelete = HudUi::ShowMessageBox(messageText, titleText, (void *)1) == 1 ? 1 : 0;
    }

    if (shouldDelete == 0) {
        return;
    }

    remove(saveGamePath);
    gameNameInput.Update("");
    RefreshSaveFileList();

    int selectedIndex = selectedEntryIndex;
    const int entryCount = SaveLoadEntryCount(this);
    if ((unsigned int)(selectedIndex) >=
        (unsigned int)(entryCount - 1)) {
        selectedIndex = entryCount - 1;
    }

    SetSelectedEntryIndex(selectedIndex);
}

// Reimplements 0x4348b0: HudUiSaveLoadGameNameInput::OnActivate
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudUiSaveLoadGameNameInput::OnActivate()
{
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

// Reimplements 0x4348f0: HudUiSaveLoadGameNameInput::OnRawKeyboardEvent
// (D:\Proj\Battlesport\hud.cpp)
int RECOIL_THISCALL HudUiSaveLoadGameNameInput::OnRawKeyboardEvent(int key)
{
    if (strchr(k_SaveGameNameAllowedChars, key) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

// Reimplements 0x435140: HudUiSaveLoadDeleteButton::OnActivate
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
void RECOIL_THISCALL HudUiSaveLoadDeleteButton::OnActivate()
{
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();
    dialog->DeleteSaveFile(1);
}

// Restores shared back-button behavior used by save/load layouts; the retail standalone
// body is HudUiMenuBackButton::OnActivate at 0x414fa0.
void RECOIL_THISCALL HudUiSaveLoadBackButton::OnActivate()
{
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x435160: HudUiSaveLoadNextButton::OnActivate
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
void RECOIL_THISCALL HudUiSaveLoadNextButton::OnActivate()
{
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int nextEntryIndex = dialog->selectedEntryIndex + 1;
    if (nextEntryIndex >= 0 && nextEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(nextEntryIndex);
    }
}

// Reimplements 0x4351b0: HudUiSaveLoadPrevButton::OnActivate
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
void RECOIL_THISCALL HudUiSaveLoadPrevButton::OnActivate()
{
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int prevEntryIndex = dialog->selectedEntryIndex - 1;
    if (prevEntryIndex >= 0 && prevEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(prevEntryIndex);
    }
}

// Reimplements 0x435220: HudUiSaveGamePrimaryActionButton::OnActivate
// (D:\Proj\Battlesport\hud.cpp)
void RECOIL_THISCALL HudUiSaveGamePrimaryActionButton::OnActivate()
{
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x435200: HudUiLoadGamePrimaryActionButton::OnActivate
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
void RECOIL_THISCALL HudUiLoadGamePrimaryActionButton::OnActivate()
{
    HudUiLoadGameDialog *const dialog = (HudUiLoadGameDialog *)(owner);
    if (dialog != 0) {
        dialog->OnPrimaryAction();
    }

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x436530: HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix(HudUiSaveLoadEntry *entryPosition,
                                                 HudUiSaveLoadEntry entry)
{
    HudUiSaveLoadEntry *writePosition = entryPosition;
    HudUiSaveLoadEntry *previous = entryPosition - 1;

    while (entry.IsNewerThan(previous) != 0) {
        *writePosition = *previous;
        writePosition = previous;
        --previous;
    }

    *writePosition = entry;
}

// Reimplements 0x436580: HudUiSaveLoadDialog::PartitionEntriesByPivot
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE HudUiSaveLoadEntry *RECOIL_FASTCALL
HudUiSaveLoadDialog::PartitionEntriesByPivot(HudUiSaveLoadEntry *begin,
                                             HudUiSaveLoadEntry *end,
                                             HudUiSaveLoadEntry pivot)
{
    HudUiSaveLoadEntry *left = begin;
    HudUiSaveLoadEntry *right = end;

    for (;;) {
        while (left->IsNewerThan(&pivot) != 0) {
            ++left;
        }

        --right;
        while (pivot.IsNewerThan(right) != 0) {
            --right;
        }

        if (right <= left) {
            break;
        }

        HudUiSaveLoadEntry temp = *left;
        *left = *right;
        ++left;
        *right = temp;
    }

    return left;
}

// Reimplements 0x4362f0: HudUiSaveLoadDialog::SortEntryRange
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
HudUiSaveLoadDialog::SortEntryRange(HudUiSaveLoadEntry *begin, HudUiSaveLoadEntry *end,
                                    int unused)
{
    (void)unused;

    HudUiSaveLoadEntry *rangeBegin = begin;
    HudUiSaveLoadEntry *rangeEnd = end;
    int entryCount = rangeEnd - rangeBegin;
    if (entryCount <= 16) {
        return;
    }

    for (;;) {
        HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
        HudUiSaveLoadEntry middleEntry = rangeBegin[entryCount / 2];
        HudUiSaveLoadEntry firstEntry = *rangeBegin;

        HudUiSaveLoadEntry *pivotSource;
        if (firstEntry.IsNewerThan(&middleEntry) != 0) {
            if (middleEntry.IsNewerThan(&lastEntry) != 0) {
                pivotSource = &middleEntry;
            } else if (firstEntry.IsNewerThan(&lastEntry) != 0) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &firstEntry;
            }
        } else {
            if (firstEntry.IsNewerThan(&lastEntry) != 0) {
                pivotSource = &firstEntry;
            } else if (middleEntry.IsNewerThan(&lastEntry) != 0) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &middleEntry;
            }
        }

        HudUiSaveLoadEntry pivotStageCopy = *pivotSource;
        HudUiSaveLoadEntry pivotEntry = pivotStageCopy;
        HudUiSaveLoadEntry *left = rangeBegin;
        HudUiSaveLoadEntry *right = rangeEnd;

        for (;;) {
            while (left->IsNewerThan(&pivotEntry) != 0) {
                ++left;
            }

            --right;
            while (pivotEntry.IsNewerThan(right) != 0) {
                --right;
            }

            if (right <= left) {
                break;
            }

            HudUiSaveLoadEntry swapTemp = *left;
            *left = *right;
            ++left;
            *right = swapTemp;
        }

        const int rightCount = rangeEnd - left;
        const int leftCount = left - rangeBegin;
        if (rightCount > leftCount) {
            SortEntryRange(rangeBegin, left, 0);
            rangeBegin = left;
        } else {
            SortEntryRange(left, rangeEnd, 0);
            rangeEnd = left;
        }

        entryCount = rangeEnd - rangeBegin;
        if (entryCount <= 16) {
            break;
        }
    }
}

// Reimplements 0x4355e0: HudUiSaveLoadDialog::RefreshSaveFileList
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::RefreshSaveFileList()
{
    HudUiSaveLoadEntries *entries = &fileEntries;
    entries->end = entries->begin;

    HudUiSaveLoadEntry findData;
    memset(&findData, 0, sizeof(findData));
    HANDLE findHandle = FindFirstFileA("SavedGames\\*.*", &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            AppendSaveLoadEntry(entries, findData);
        }

        while (FindNextFileA(findHandle, &findData) != 0) {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                AppendSaveLoadEntry(entries, findData);
            }
        }
    }

    HudUiSaveLoadEntry *begin = entries->begin;
    HudUiSaveLoadEntry *end = entries->end;
    const int entryCount = end - begin;

    if (entryCount > 16) {
        HudUiSaveLoadEntry *rangeBegin = begin;
        HudUiSaveLoadEntry *rangeEnd = end;
        int rangeCount = entryCount;

        do {
            HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
            HudUiSaveLoadEntry middleEntry = rangeBegin[rangeCount / 2];
            HudUiSaveLoadEntry firstEntry = *rangeBegin;

            HudUiSaveLoadEntry *pivotSource;
            if (firstEntry.IsNewerThan(&middleEntry) != 0) {
                if (middleEntry.IsNewerThan(&lastEntry) != 0) {
                    pivotSource = &middleEntry;
                } else if (firstEntry.IsNewerThan(&lastEntry) != 0) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &firstEntry;
                }
            } else {
                if (firstEntry.IsNewerThan(&lastEntry) != 0) {
                    pivotSource = &firstEntry;
                } else if (middleEntry.IsNewerThan(&lastEntry) != 0) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &middleEntry;
                }
            }

            HudUiSaveLoadEntry pivot = *pivotSource;
            HudUiSaveLoadEntry *split = PartitionEntriesByPivot(rangeBegin, rangeEnd, pivot);
            const int leftCount = split - rangeBegin;
            const int rightCount = rangeEnd - split;
            if (rightCount > leftCount) {
                SortEntryRange(rangeBegin, split, 0);
                rangeBegin = split;
            } else {
                SortEntryRange(split, rangeEnd, 0);
                rangeEnd = split;
            }

            rangeCount = rangeEnd - rangeBegin;
        } while (rangeCount > 16);
    }

    if (entryCount <= 16) {
        if (begin == end) {
            return;
        }

        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition == end) {
            return;
        }

        do {
            HudUiSaveLoadEntry entry = *entryPosition;
            if (entry.IsNewerThan(begin) != 0) {
                HudUiSaveLoadEntry *writePosition = entryPosition;
                while (writePosition != begin) {
                    *writePosition = *(writePosition - 1);
                    --writePosition;
                }
                *begin = entry;
            } else {
                InsertEntryIntoSortedPrefix(entryPosition, entry);
            }
            ++entryPosition;
        } while (entryPosition != end);
        return;
    }

    HudUiSaveLoadEntry *firstBlockEnd = begin + 16;
    if (begin != firstBlockEnd) {
        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition != firstBlockEnd) {
            do {
                HudUiSaveLoadEntry entry = *entryPosition;
                if (entry.IsNewerThan(begin) != 0) {
                    HudUiSaveLoadEntry *writePosition = entryPosition;
                    while (writePosition != begin) {
                        *writePosition = *(writePosition - 1);
                        --writePosition;
                    }
                    *begin = entry;
                } else {
                    InsertEntryIntoSortedPrefix(entryPosition, entry);
                }
                ++entryPosition;
            } while (entryPosition != firstBlockEnd);
        }
    }

    for (HudUiSaveLoadEntry *entryPosition = firstBlockEnd; entryPosition != end; ++entryPosition) {
        HudUiSaveLoadEntry entry = *entryPosition;
        HudUiSaveLoadEntry *previous = entryPosition - 1;
        HudUiSaveLoadEntry *writePosition = entryPosition;
        if (entry.IsNewerThan(previous) != 0) {
            do {
                *writePosition = *previous;
                writePosition = previous;
                --previous;
            } while (entry.IsNewerThan(previous) != 0);
            *writePosition = entry;
        }
    }
}

// Reimplements 0x434ee0: HudUiSaveLoadDialog::InitializeFileEntries
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::InitializeFileEntries()
{
    entryWidgets[0].layoutY = 9830;
    entryWidgets[1].layoutY = 16383;
    entryWidgets[2].layoutY = 32767;
    entryWidgets[3].layoutY = 32767;
    entryWidgets[4].layoutY = 32767;
    entryWidgets[5].layoutY = 29490;
    entryWidgets[6].layoutY = 22936;
    entryWidgets[7].layoutY = 16383;
    entryWidgets[8].layoutY = 9830;

    RefreshSaveFileList();

    HudUiSaveLoadEntry *entry = fileEntries.begin;
    int index = 0;
    while (index < 9 && entry != fileEntries.end) {
        HudUiSaveLoadListItem *listItem = &entryWidgets[index];
        listItem->layoutX = index;
        listItem->vftable->SetTextFmt(listItem, "%s", entry->cFileName);
        listItem->vftable->SetVisible(listItem, 1);

        ++entry;
        ++index;
    }
}

// Reimplements 0x4353f0: HudUiSaveLoadDialog::SetSelectedEntryIndex
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::SetSelectedEntryIndex(int selectedEntryIndexValue)
{
    selectedEntryIndex = selectedEntryIndexValue;
    const int entryCount = SaveLoadEntryCount(this);

    for (int row = 0; row < 3; ++row) {
        const int entryIndex = selectedEntryIndexValue + row - 3;
        HudUiSaveLoadListItem *listItem = &entryWidgets[row];
        if (entryIndex >= 0 && entryIndex < entryCount) {
            listItem->layoutX = entryIndex;
            listItem->vftable->SetTextFmt(listItem, "%s", fileEntries.begin[entryIndex].cFileName);
            listItem->vftable->SetVisible(listItem, 1);
            listItem->vftable->Invalidate(listItem);
        } else {
            listItem->vftable->SetVisible(listItem, 0);
        }
    }

    if (selectedEntryIndexValue >= 0 && selectedEntryIndexValue < entryCount) {
        gameNameInput.Update(fileEntries.begin[selectedEntryIndexValue].cFileName);
    }

    for (int lowerRow = 3; lowerRow < 9; ++lowerRow) {
        const int entryIndex = selectedEntryIndexValue + lowerRow - 2;
        HudUiSaveLoadListItem *listItem = &entryWidgets[lowerRow];
        if (entryIndex >= 0 && entryIndex < entryCount) {
            listItem->layoutX = entryIndex;
            listItem->vftable->SetTextFmt(listItem, "%s", fileEntries.begin[entryIndex].cFileName);
            listItem->vftable->SetVisible(listItem, 1);
            listItem->vftable->Invalidate(listItem);
        } else {
            listItem->vftable->SetVisible(listItem, 0);
        }
    }
}

// Reimplements 0x435a70: HudUiSaveLoadDialog::ProcessDialogResult
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::ProcessDialogResult()
{
    char *const gameName = gameNameInput.GetBuffer();
    char saveGamePath[MAX_PATH];
    saveGamePath[0] = '\0';

    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    if (zUtil::ZAR_LoadFileGlobal(saveGamePath) == 0) {
        return;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    zSndPlayHandleSnapshot *const snapshot =
        (zSndPlayHandleSnapshot *)((unsigned int)(
            g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot));
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot = 0;
    }

    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    switch (g_RecoilStateSaveLoadTransition.m_transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        if (saveGamePath[0] != '\0') {
            g_RecoilApp.m_playState_208.pPendingLoadGameStartPath =
                (RecoilPtr32)((unsigned int)(_strdup(saveGamePath)));
            g_RecoilApp.m_missionFmvState_1d8.m_skipMissionFmv = 1;
            g_RecoilApp.QueueExitCurrentState(1);
            g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState_1d8.base, 0);
        } else {
            g_RecoilApp.QueueExitCurrentState(0);
        }
        break;

    case RECOIL_SAVELOAD_MODE_FADE:
        ExtendPlayStateTransitionTimer(5.0f);
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.QueueExitCurrentState(1);
        break;

    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        ExtendPlayStateTransitionTimer(5.0f);
        g_RecoilApp.QueueExitCurrentState(0);
        break;
    }
}

// Reimplements 0x434dc0: HudUiLoadGameDialog::ProcessDialogResult
// (D:\Proj\Battlesport\HudUiLoadGameDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiLoadGameDialog::ProcessDialogResult()
{
    HudUiSaveLoadDialog::ProcessDialogResult();
}

// Reimplements 0x435240: HudUiLoadGameDialog::OnPrimaryAction
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiLoadGameDialog::OnPrimaryAction()
{
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        g_RecoilApp.QueueExitCurrentState(0);
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(titleText, zLoc::GetMessageString(136));
        strcpy(messageText, zLoc::GetMessageString(137));
        if (HudUi::ShowMessageBox(messageText, titleText, (void *)1) == 2) {
            return;
        }
    }

    while (zUtil::ZBD_LoadEntriesGlobal(saveGamePath) == 0) {
        DeleteSaveFile(0);

        char titleText[128];
        char messageText[128];
        strcpy(titleText, zLoc::GetMessageString(136));
        strcpy(messageText, zLoc::GetMessageString(140));
        if (HudUi::ShowMessageBox(messageText, titleText, (void *)1) == 2) {
            break;
        }
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

// Reimplements 0x434680: HudUiSaveGameDialog::InitLayout
// (D:\Proj\Battlesport\hudui_saveload.cpp)
RECOIL_NOINLINE HudUiSaveGameDialog *RECOIL_THISCALL
HudUiSaveGameDialog::InitLayout()
{
    base.Constructor();

    deleteButton.Constructor();
    deleteButton.base.ftable = &g_HudUiSaveLoad_DeleteButton_Vtbl;
    backButton.Constructor();
    backButton.base.ftable = &g_HudUiSaveLoad_BackButton_Vtbl;
    nextEntryButton.Constructor();
    nextEntryButton.base.ftable = &g_HudUiSaveLoad_NextButton_Vtbl;
    prevEntryButton.Constructor();
    prevEntryButton.base.ftable = &g_HudUiSaveLoad_PrevButton_Vtbl;

    gameNameInput.BaseConstructor();
    gameNameInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiSaveLoadGameNameInput_Vtbl);
    gameNameInput.textInput.AllocTextBuffer(20);
    gameNameInput.Update("");
    gameNameInput.SetInputActive(1);
    gameNameInput.SetRawKeyboardCapture(1);

    for (int i = 0; i < 9; ++i) {
        entryWidgets[i].Constructor();
    }

    fileEntries.reserved00 = 0;
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;
    base.base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiSaveLoadDialog_Vtbl);

    primaryActionButton.Constructor();
    primaryActionButton.base.ftable = &g_HudUiSaveGame_PrimaryActionButton_Vtbl;
    base.base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiSaveGameDialog_Vtbl);

    zReader::Node *const loadedSection =
        base.LoadFromZrd("dialog.zrd", "SAVE_GAME_DIALOG", 0);
    if (loadedSection != 0) {
        base.BindWidgetByName(loadedSection, &backButton.base, "BACK");
        base.BindWidgetByName(loadedSection, &nextEntryButton.base, "NEXT_GAME_BTN");
        base.BindWidgetByName(loadedSection, &prevEntryButton.base, "PREV_GAME_BTN");
        base.BindWidgetByName(loadedSection, &deleteButton.base, "DELETE_BTN");
        base.BindWidgetByName(loadedSection, &primaryActionButton.base, "SAVE");
        base.BindWidgetByName(loadedSection, &gameNameInput.base.base, "GAMENAME");

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(listNodeName, "LIST_%d", i);
            base.BindPrimitiveNodeToElement(
                loadedSection, (HudUiElement *)(&entryWidgets[i]), listNodeName);
        }

        base.FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(-1);
    return this;
}

// Reimplements 0x434a80: HudUiSaveGameDialog::Destructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveGameDialog::Destructor()
{
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        ((HudUiPanel *)(&entryWidgets[index - 1]))->Destructor();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    base.Destructor();
}

// Reimplements 0x434b90: HudUiLoadGameDialog::Constructor
// (D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp)
RECOIL_NOINLINE HudUiLoadGameDialog *RECOIL_THISCALL
HudUiLoadGameDialog::Constructor()
{
    base.Constructor();

    deleteButton.Constructor();
    deleteButton.base.ftable = &g_HudUiSaveLoad_DeleteButton_Vtbl;
    backButton.Constructor();
    backButton.base.ftable = &g_HudUiSaveLoad_BackButton_Vtbl;
    nextEntryButton.Constructor();
    nextEntryButton.base.ftable = &g_HudUiSaveLoad_NextButton_Vtbl;
    prevEntryButton.Constructor();
    prevEntryButton.base.ftable = &g_HudUiSaveLoad_PrevButton_Vtbl;

    gameNameInput.BaseConstructor();
    gameNameInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiSaveLoadGameNameInput_Vtbl);
    gameNameInput.textInput.AllocTextBuffer(20);
    gameNameInput.Update("");
    gameNameInput.SetInputActive(1);
    gameNameInput.SetRawKeyboardCapture(1);

    for (int i = 0; i < 9; ++i) {
        entryWidgets[i].Constructor();
    }

    fileEntries.reserved00 = 0;
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;
    base.base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiSaveLoadDialog_Vtbl);

    primaryActionButton.Constructor();
    primaryActionButton.base.ftable = &g_HudUiLoadGame_PrimaryActionButton_Vtbl;
    base.base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiLoadGameDialog_Vtbl);

    zReader::Node *const loadedSection =
        base.LoadFromZrd("dialog.zrd", "LOAD_GAME_DIALOG", 0);
    if (loadedSection != 0) {
        base.BindWidgetByName(loadedSection, &backButton.base, "BACK");
        base.BindWidgetByName(loadedSection, &nextEntryButton.base, "NEXT_GAME_BTN");
        base.BindWidgetByName(loadedSection, &prevEntryButton.base, "PREV_GAME_BTN");
        base.BindWidgetByName(loadedSection, &deleteButton.base, "DELETE_BTN");
        base.BindWidgetByName(loadedSection, &primaryActionButton.base, "LOAD");
        base.BindWidgetByName(loadedSection, &gameNameInput.base.base, "GAMENAME");

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(listNodeName, "LIST_%d", i);
            base.BindPrimitiveNodeToElement(
                loadedSection, (HudUiElement *)(&entryWidgets[i]), listNodeName);
        }

        base.FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(0);

    return this;
}

// Reimplements 0x4349a0: HudUiSaveLoadDialog::Destructor
// (D:\Proj\Battlesport\hud.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiSaveLoadDialog::Destructor()
{
    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        ((HudUiPanel *)(&entryWidgets[index - 1]))->Destructor();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    base.Destructor();
}

// Reimplements 0x434df0: HudUiLoadGameDialog::Destructor
// (D:\Proj\Battlesport\HudUiLoadGameDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
HudUiLoadGameDialog::Destructor()
{
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        ((HudUiPanel *)(&entryWidgets[index - 1]))->Destructor();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    base.Destructor();
}

// Reimplements 0x434dd0: HudUiLoadGameDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiLoadGameDialog.cpp)
RECOIL_NOINLINE HudUiLoadGameDialog *RECOIL_THISCALL
HudUiLoadGameDialog::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x435d20: RecoilStateSaveLoadTransition::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
RecoilStateSaveLoadTransition::OnTryBecomeCurrent()
{
    if (m_capturePresentationMode != RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        if (g_zVideo_ActiveRendererPath != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
        }

        m_savedHalfResAdjustMode =
            (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);
        zSnd::ApplyMuteStateToActiveVoices(1);

        zSndPlayHandleSnapshot *const audioSnapshot =
            zSndPlayHandleSnapshot::CreateFromActiveSamples();
        m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
        audioSnapshot->StopAllIfPlaying();

        zFMV_ActionBlur blurAction;
        blurAction.Constructor(4, 1);
        blurAction.vftable->Begin(&blurAction, 0.0);
        while (blurAction.vftable->Update(&blurAction, 0.0) != 0) {
        }
        blurAction.vftable->End(&blurAction);
        blurAction.vftable = &g_zFMV_ActionBase_Vtable;

        zSndSampleSet_InitByName("DIALOG");
    }

    HudUiSaveLoadDialog *dialog = 0;
    if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
        HudUiSaveGameDialog *const storage =
            (HudUiSaveGameDialog *)::operator new(sizeof(HudUiSaveGameDialog));
        if (storage != 0) {
            dialog = storage->InitLayout();
        }
    } else {
        HudUiLoadGameDialog *const storage =
            (HudUiLoadGameDialog *)::operator new(sizeof(HudUiLoadGameDialog));
        if (storage != 0) {
            dialog = storage->Constructor();
        }
    }

    m_dialog = (RecoilPtr32)(unsigned int)dialog;
    dialog->base.base.base.SetEnabled(1);
    return 1;
}

// Reimplements 0x435e80: RecoilStateSaveLoadTransition::OnUpdateShouldQuit
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
RecoilStateSaveLoadTransition::OnUpdateShouldQuit()
{
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiSaveLoadDialogUpdateDispatch *const dialog =
            (HudUiSaveLoadDialogUpdateDispatch *)((unsigned int)(m_dialog));
        dialog->Update(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled((zVidRect32 *)srcRect, (zVidRect32 *)dstRect, 1, 1);
    return 0;
}

// Reimplements 0x435ed0: RecoilStateSaveLoadTransition::OnDeactivate
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
RecoilStateSaveLoadTransition::OnDeactivate()
{
    if (m_dialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        RecoilStateSaveLoadDialogVirtual *dialog =
            (RecoilStateSaveLoadDialogVirtual *)((unsigned int)m_dialog);
        dialog->SetEnabled(0);

        ((HudUiDialogController *)((unsigned int)m_dialog))->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = (RecoilStateSaveLoadDialogVirtual *)((unsigned int)m_dialog);
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }

    if (m_capturePresentationMode == RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        return;
    }

    zSndSampleSet_DestroyByName("DIALOG");

    zSndPlayHandleSnapshot *const audioSnapshot =
        (zSndPlayHandleSnapshot *)((unsigned int)m_pausedAudioSnapshot);
    if (audioSnapshot != 0) {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zSnd::ApplyMuteStateToActiveVoices(0);
    zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x435f50: RecoilStateSaveLoadTransition::QueueOpenSaveDialog
// (D:\Proj\Battlesport\RecoilApp.cpp)
void RECOIL_FASTCALL
RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
    RecoilSaveLoadPresentationCaptureMode capturePresentationMode)
{
    if (HudUiMainMenuDialog::CanSaveGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = capturePresentationMode;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

// Reimplements 0x435f80: RecoilStateSaveLoadTransition::QueueOpenLoadDialog
// (D:\Proj\Battlesport\RecoilApp.cpp)
void RECOIL_FASTCALL
RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode)
{
    if (HudUiMainMenuDialog::CanLoadGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_transitionMode = transitionMode;
    switch (transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        break;
    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
        break;
    }

    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

// Reimplements 0x430c90: RecoilApp::FatalErrorAndExit (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL
RecoilApp::FatalErrorAndExit(int errorCode) {
    if (errorCode != -1) {
        return;
    }

    char caption[0x80];
    char text[0x80];
    strcpy(caption, zLoc::GetMessageString(0x12));
    strcpy(text, zLoc::GetMessageString(0x30));

    Briefing::StopAndShutdownThread(0);
    zVideo_dd::FlipToGDIIfAttached();
    zSndSystem::Shutdown();
    zNetwork::ShutdownSessionRuntime();
    zVideo::ShutdownVideoSystem();
    printf("%s: %s\n", caption, text);
    Sleep(1000);
    MessageBeep(MB_ICONHAND);
    MessageBoxA(g_RecoilApp_hWndMain, text, caption, MB_ICONHAND);
    zSys::ExitProcessWithCleanup(0);
}

// Reimplements 0x42e930: RecoilApp::ExitInstance
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::ExitInstance() {
    if (g_RecoilApp_WindowClassRegistered != 0) {
        UnregisterClassA(g_RecoilApp_WndClassNamePtr,
                         AfxGetModuleState()->m_hCurrentInstanceHandle);
        zGame::Options_SaveGameOptions();
        zGame::ReturnOnlyStub();
        zGame::Options_ShutdownRegistryContext();
        zUtil_ZRDR_Shutdown();
        zUtil_ZRDR_FreeNodePool();
        zUtil::ZBD_DestroyGlobalManager();
        zLoc::UnloadMessagesDll();
    }

    zInput::BindMapSystem_Shutdown();
    ((CWinApp *)(this))->CWinApp::ExitInstance();
    zSys::ExitProcessWithCleanup(0);
    return 0;
}

// Reimplements 0x42e520: RecoilApp::InitInstance
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL RecoilApp::InitInstance() {
    if (ActivateExistingInstance() == 0) {
        return 0;
    }

    WNDCLASSA wndClass = {0};
    wndClass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = AfxGetModuleState()->m_hCurrentInstanceHandle;
    wndClass.hIcon =
        LoadIconA(AfxFindResourceHandle(IntResource(0x97), IntResource(0x0e)), IntResource(0x97));
    wndClass.hCursor = LoadCursorA(AfxFindResourceHandle(IntResource(0x7f00), IntResource(0x0c)),
                                   IntResource(0x7f00));
    wndClass.hbrBackground = CreateSolidBrush(0);
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = g_RecoilApp_WndClassNamePtr;

    if (AfxRegisterClass(&wndClass) == 0) {
        return 0;
    }

    g_RecoilApp_WindowClassRegistered = 1;
    InitMainWindow();
    m_reserved148 = 0;
    m_pendingState_0c4 =
        (RecoilPtr32)((unsigned int)(&m_introFmvState_1a0));

    char errorTextBuffer[0x400];
    char messageCaptionBuffer[0x100];
    char sharedTextBuffer[0x100];
    char registryCompanyNameBuffer[0x100];

    if (zLoc::LoadMessagesDll("MESSAGES.DLL") == 0) {
        char *systemErrorText = 0;
        sprintf(errorTextBuffer, "Exit at %s:%d\n", "D:\\Proj\\Battlesport\\RecoilApp.cpp",
                     0x188);
        OutputDebugStringA(errorTextBuffer);
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0,
                       GetLastError(), 0x400, (LPSTR)(&systemErrorText), 0,
                       0);
        strcpy(errorTextBuffer, systemErrorText);
        strcat(errorTextBuffer, "\n\n");
        strcat(errorTextBuffer, "MESSAGES.DLL");
        LocalFree(systemErrorText);
        zVideo_dd::FlipToGDIIfAttached();
        MessageBeep(MB_ICONASTERISK);
        MessageBoxA(0, errorTextBuffer, "", MB_ICONASTERISK);
        ExitProcess(0);
    }

    zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x83);
    while (zSys::FindFileOnDriveType(5, "video\\intro_01.avi", 0) == 0) {
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(g_RecoilApp_hWndMain, messageCaptionBuffer, zLoc::GetMessageString(0x901),
                        MB_OKCANCEL | MB_ICONEXCLAMATION) != IDOK) {
            ExitProcess(0);
        }
    }

    zSysVideoCapsLevel videoCaps = ZSYS_VIDEO_CAPS_NONE;
    zSysPlatformCapsLevel platformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
    zSys::ProbePlatformAndVideoCaps(&videoCaps, &platformCaps);
    if ((unsigned int)(videoCaps) <
        (unsigned int)(ZSYS_VIDEO_CAPS_SURFACE4)) {
        zLoc::FormatMessage(messageCaptionBuffer, 0x100, 0x14);
        zLoc::FormatMessage(sharedTextBuffer, 0x100, 0x16);
        MessageBeep(MB_ICONHAND);
        MessageBoxA(g_RecoilApp_hWndMain, sharedTextBuffer, messageCaptionBuffer, MB_ICONHAND);
        ExitProcess(0);
    }

    zGame::ReturnOnlyStub();
    zUtil::ZBD_Init();
    zUtil::ZRDR_PreallocNodePool(0x200);
    zUtil::ZRDR_AddSearchPaths(0, "zbd");
    zUtil::ZRDR_Init("..\\data\\common\\zrdr");

    strncpy(registryCompanyNameBuffer, zLoc::GetMessageString(0x900),
                 sizeof(registryCompanyNameBuffer));
    strncpy(sharedTextBuffer, zLoc::GetMessageString(0x901), sizeof(sharedTextBuffer));
    zGame::Options_InitRegistryContext(registryCompanyNameBuffer, sharedTextBuffer,
                                       RecoilVersion::GetString());
    zInput::BindMapSystem_Init(0x2f);

    if (zGame::Options_LoadGameOptions() == 0) {
        zArchive::MountIndexArchive("zbd\\zrdr.zbd", 1);
        if (zGame::Options_LoadGameOptions() == 0) {
            strcpy(sharedTextBuffer, zLoc::GetMessageString(0x901));
            MessageBeep(MB_ICONHAND);
            MessageBoxA(g_RecoilApp_hWndMain, zLoc::GetMessageString(0x1e), sharedTextBuffer,
                        MB_ICONHAND);
            ExitProcess(0);
        }
    }

    zVid::SetVideoModeIndex(zVid::GetVideoModeIndexFromOptions());
    CZRecoilFrame *const frame = (CZRecoilFrame *)((unsigned int)(GetMainWnd()));
    frame->ConfigureModeFeatureFlags();
    ((CZRecoilFrame *)((unsigned int)(GetMainWnd())))->InitStartupHwApiFromOptions();
    return 1;
}

// Reimplements 0x42e110: RecoilApp::CreateMainWnd
RECOIL_NOINLINE CZRecoilFrame *RECOIL_THISCALL RecoilApp::CreateMainWnd() {
    CZRecoilFrame *frame = new CZRecoilFrame;
    if (frame == 0) {
        return 0;
    }

    return frame->Constructor();
}

// Reimplements 0x4429d0: RecoilApp::InitMainWindow
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::InitMainWindow() {
    ((CWinApp *)(this))->Enable3dControls();

    typedef CZRecoilFrame * (RECOIL_THISCALL *CreateMainWndMethod)(RecoilApp *);
    const CreateMainWndMethod *const createMainWnd = (const CreateMainWndMethod *)((unsigned int)(vftable + 0xb8));
    m_pMainWnd = (RecoilPtr32)((unsigned int)((*createMainWnd)(this)));
    CZRecoilFrame *const mainWnd = (CZRecoilFrame *)((unsigned int)(GetMainWnd()));
    mainWnd->m_app = (RecoilPtr32)((unsigned int)(this));
    ((CWnd *)((unsigned int)(m_pMainWnd)))->ShowWindow(SW_SHOW);
    UpdateWindow(((CWnd *)((unsigned int)(m_pMainWnd)))->m_hWnd);
    return 1;
}

namespace {
typedef void (RECOIL_THISCALL *RecoilStateMethod)(RecoilApp_IState *);
typedef int (RECOIL_THISCALL *RecoilStateIntMethod)(RecoilApp_IState *);
typedef void (RECOIL_THISCALL *RecoilStateParamMethod)(RecoilApp_IState *, int);
typedef int (RECOIL_THISCALL *RecoilStateIdleMethod)(RecoilApp_IState *, unsigned int,
                                                              unsigned int);
typedef int (RECOIL_THISCALL *RecoilAppNoArgIntMethod)(RecoilApp *);
typedef void (RECOIL_THISCALL *RecoilAppNoArgVoidMethod)(RecoilApp *);
typedef int (RECOIL_THISCALL *RecoilAppStartEngineMethod)(RecoilApp *, RecoilPtr32);

const char kEngineInitFailed[] = "FAILED";
const char kEngineInitPassed[] = "PASSED";

inline void PrintEngineInitZeroStatus(const char *format, int result) {
    printf(format, result == 0 ? kEngineInitPassed : kEngineInitFailed);
}

inline void PrintEngineInitNonzeroStatus(const char *format, int result) {
    printf(format, result != 0 ? kEngineInitPassed : kEngineInitFailed);
}

void CallRecoilStateMethod(RecoilPtr32 stateValue, size_t vtableOffset) {
    RecoilApp_IState *const state = (RecoilApp_IState *)((unsigned int)(stateValue));
    const RecoilPtr32 methodValue = *(const RecoilPtr32 *)(
        (unsigned int)(state->vftable + vtableOffset));
    RecoilStateMethod const method = (RecoilStateMethod)((unsigned int)(methodValue));
    method(state);
}

int CallRecoilStateIntMethod(RecoilPtr32 stateValue, size_t vtableOffset) {
    RecoilApp_IState *const state = (RecoilApp_IState *)((unsigned int)(stateValue));
    const RecoilPtr32 methodValue = *(const RecoilPtr32 *)(
        (unsigned int)(state->vftable + vtableOffset));
    RecoilStateIntMethod const method =
        (RecoilStateIntMethod)((unsigned int)(methodValue));
    return method(state);
}

void CallRecoilStateParamMethod(RecoilPtr32 stateValue, size_t vtableOffset, int param) {
    RecoilApp_IState *const state = (RecoilApp_IState *)((unsigned int)(stateValue));
    const RecoilPtr32 methodValue = *(const RecoilPtr32 *)(
        (unsigned int)(state->vftable + vtableOffset));
    RecoilStateParamMethod const method =
        (RecoilStateParamMethod)((unsigned int)(methodValue));
    method(state, param);
}

RecoilFn32 ReadRecoilVtableSlot(RecoilPtr32 vftable, size_t offset) {
    return *(const RecoilFn32 *)((unsigned int)(vftable + offset));
}
} // namespace

// Reimplements 0x442a50: RecoilApp::EngineInit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::EngineInit(RecoilPtr32 hwnd) {
    zUtil::ZRDR_PreallocNodePool(0);
    zUtil::ZRDR_Init(0);

    PrintEngineInitZeroStatus("gModInit:  %s\n", zModel_Display_Init());
    PrintEngineInitZeroStatus("gClsInit:  %s\n", zVideo::ReturnSuccessStub());
    PrintEngineInitZeroStatus("zEffInit:  %s\n", zEffect::Init());
    PrintEngineInitZeroStatus("zRndrInit: %s\n", zRndr::InitGlobals());
    PrintEngineInitNonzeroStatus("zSndInit:  %s\n", zSnd_PreInitializeRuntimeState(hwnd));
    PrintEngineInitZeroStatus("zUtlInit:  %s\n", zVideo::ReturnSuccessStub());
    PrintEngineInitZeroStatus("zWepInit:  %s\n", zWepInit());
    PrintEngineInitZeroStatus("zImgInit:  %s\n", zImage_Init(0));

    if (g_zVideo_ActiveRendererPath == 2) {
        zInput::Mouse_SetCooperativeLevelFlags(5);
    }

    PrintEngineInitZeroStatus("zInInit:  %s\n",
                          zInput::Init((HWND)((unsigned int)(hwnd)),
                                       (HINSTANCE)(
                                           (unsigned int)(m_hInstance_6c))));
    Time::Reset();
    zVid::SetCachedClientRectUpdateMask(1);
    return 1;
}

// Reimplements 0x42e330: RecoilApp::InitializeDisplay
RECOIL_NOINLINE int RECOIL_FASTCALL RecoilApp::InitializeDisplay(RecoilPtr32 hwnd) {
    const int modeIndex = zVid::GetVideoModeIndexFromOptions();
    const int fullscreen = zOpt::GetFullscreenOption();
    if (zVideo::InitVideoSystem((HWND)((unsigned int)(hwnd)),
                                zVid::GetHwApiOption(), fullscreen, modeIndex) != 0) {
        printf("Error opening video... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    if (zVid::GetAccelerationOption() == 0 &&
        zRndr::SpanOcclusionInit(zOpt::GetWindowSectionHeight()) != 0) {
        printf("Error opening HSE... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const activeRegionRect = zOpt::GetDisplaySection();
    void *const primaryPixels = zVideo::GetPrimarySurfacePixels();
    zRndr::SetFrameBufferRegion(primaryPixels, activeRegionRect, bitsPerPixel, pitchBytes);
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());
    zVid::InitFrameScratchBuffers();

    const int oldClearState = zVideo::ExchangeClearScreenBufferEnabled(1);
    zVideo::CallClearSwSurfaceAndZBuffer(0, 0);
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(0, 0, 1, 1);
    zVideo::ExchangeClearScreenBufferEnabled(oldClearState);
    return 1;
}

// Reimplements 0x42e220: RecoilApp::StartEngine
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_THISCALL RecoilApp::StartEngine(RecoilPtr32 hwnd) {
    EngineInit(hwnd);
    PrintEngineInitZeroStatus("turret:    %s\n", zTurret_System::ResetIterationState());

    zSndSystem_Init(hwnd, "sounds.zrd");
    zSnd::SetAudioApiOption(zSnd::GetActiveBackend());

    if (InitializeDisplay(hwnd) == 0) {
        char caption[0x80];
        strcpy(caption, zLoc::GetMessageString(0x901));
        MessageBoxExA((HWND)((unsigned int)(hwnd)),
                      zLoc::GetMessageString(0x1f), caption, MB_ICONHAND, 0);
        return 0;
    }

    zInput::Init((HWND)((unsigned int)(hwnd)), g_RecoilApp_hInstance);
    const int height = zOpt_DisplaySection_GetHeight();
    zInput::Mouse_SetClientSizeAndCenter(zOpt_DisplaySection_GetWidth(), height);
    zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption());

    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    HudUiMgr::InitHudLayouts((const HudUiRect *)(zOpt::GetDisplaySection()),
                             (const HudUiRect *)(windowSection));
    return 1;
}

// Reimplements 0x42e990: RecoilApp::ActivateExistingInstance
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::ActivateExistingInstance() {
    CWnd *const existingWindow =
        CWnd::FromHandle(FindWindowA(g_RecoilApp_WndClassNamePtr, 0));
    if (existingWindow == 0) {
        return 1;
    }

    CWnd *const popup = CWnd::FromHandle(GetLastActivePopup(existingWindow->m_hWnd));
    if (IsIconic(existingWindow->m_hWnd) != 0) {
        existingWindow->ShowWindow(SW_RESTORE);
    }

    SetForegroundWindow(popup->m_hWnd);
    return 0;
}

// Reimplements 0x42e9f0: RecoilApp::PreTranslateMessage
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::PreTranslateMessage(tagMSG *msg) {
    if (zVid::GetAccelerationOption() != 0) {
        const UINT message = msg->message;
        if (message >= WM_SYSKEYDOWN && message <= WM_SYSKEYUP) {
            return 1;
        }
    }

    return 0;
}

namespace zSndCd {
void RECOIL_FASTCALL OnMciNotify(unsigned int wParam, unsigned int lParam);
}

namespace zDEClient {
RECOIL_NOINLINE int RECOIL_CDECL ShutdownGlobals();
}

// Reimplements 0x442bc0: RecoilApp::ShutdownSubsystems
RECOIL_NOINLINE int RECOIL_CDECL RecoilApp::ShutdownSubsystems() {
    zInput::Shutdown();
    zImage::ShutdownSubsystem();
    zUtil_ZRDR_ShutdownWildcardPath();
    zVid::ShutdownFrameScratchBuffers();
    zEffect::ShutdownAll();
    OptCatalog::Shutdown();
    zClass::Shutdown();
    zModel_Display::ShutdownThunk();
    zSndSystem::Shutdown();
    zUtil_ZRDR_Shutdown();
    zUtil_ZRDR_FreeNodePool();
    return 0;
}

// Reimplements 0x42e430: RecoilApp::ShutdownEngine
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::ShutdownEngine() {
    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::Stop();
    }

    zTurret_System::Shutdown();
    zDEClient::ShutdownGlobals();

    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionShutdown();
    }

    PickupTypeTable::FreeOptMeta();
    HudUiMgr::ShutdownResources();

    if (zOpt::GetNetworkEnabled() != 0) {
        zNetwork::ShutdownSessionRuntime();
    }

    ShutdownSubsystems();
    zVideo::ShutdownVideoSystem();
    zVideo::ReturnSuccessStub();
}

// Reimplements 0x42e490: RecoilApp::LoadZbdAndStartEngine
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::LoadZbdAndStartEngine() {
    if (g_HudSensorTracker.missionFlags != 0) {
        zArchive::MountIndexArchive("zbd\\zrdr.zbd", 1);
    }

    StartEngineAndQueueStartupState();
    g_HudSensorTracker.RegisterMissionSectionHandlers();
    return 1;
}

// Reimplements 0x42e4d0: RecoilApp::LoadZbdAndSetupSensorTracker
RECOIL_NOINLINE int RECOIL_THISCALL
RecoilApp::LoadZbdAndSetupSensorTracker(int missionId, const char *zbdPath,
                                        int skipIntroFmvMode, int missionFlags) {
    LoadZbdAndStartEngine();
    m_skipIntroFmv = skipIntroFmvMode;
    if (zbdPath != 0) {
        g_HudSensorTracker.SetZbdPath(zbdPath);
        return 1;
    }

    g_HudSensorTracker.InitMissionIdAndFlags(missionId, missionFlags);
    return 1;
}

extern const zMfcMsgMapEntry g_RecoilApp_MessageEntries[1] = {0};

extern const MfcMsgMap g_RecoilApp_MessageMap = {
    0x004428a0,
    0x004d0998,
};

// Reimplements 0x42de20: RecoilApp::StaticInitAndRegisterAtExit
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x42de30: RecoilApp::StaticInit
RECOIL_NOINLINE RecoilApp *RECOIL_CDECL RecoilApp::StaticInit() {
    return g_RecoilApp.Constructor();
}

// Reimplements 0x42de40: RecoilApp::RegisterAtExit
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x42de50: RecoilApp::AtExitDestructor
RECOIL_NOINLINE void RECOIL_CDECL RecoilApp::AtExitDestructor() {
    g_RecoilApp.Destructor();
}

// Reimplements 0x4a5780: RecoilApp::InitStdLogFiles
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_FASTCALL RecoilApp::InitStdLogFiles(const char *exePath) {
    g_RecoilApp_hWndMain = 0;
    if (exePath == 0) {
        return;
    }

    char pathBuf[0x40];
    strcpy(pathBuf, exePath);
    strcat(pathBuf, ".err");
    FILE *stream = freopen(pathBuf, "w", stderr);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, "gamez.err");
        stream = freopen(pathBuf, "w", stderr);
    }
    if (stream != 0) {
        fprintf(stream, "File started\n---\n");
        fflush(stream);
    }

    strcpy(pathBuf, exePath);
    strcat(pathBuf, ".out");
    stream = freopen(pathBuf, "w", stdout);
    if (stream == 0 && GetTempPathA(sizeof(pathBuf), pathBuf) != 0) {
        strcat(pathBuf, "gamez.out");
        stream = freopen(pathBuf, "w", stdout);
    }
    if (stream != 0) {
        fprintf(stream, "File started\n---\n");
        fflush(stream);
    }
}

// Reimplements 0x42dfa0: RecoilApp::Constructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL RecoilApp::Constructor() {
    MfcOleModuleConstructor();
    m_attractFmvState_160.Constructor();

    m_introFmvState_1a0.base.vftable = kRecoilStateBase_VtblAddress;
    ((zFMV_Script *)(m_introFmvState_1a0.m_fmv_08))->Init(0, 0, 0);
    m_introFmvState_1a0.base.vftable = kRecoilApp_IntroFmvState_VtblAddress;

    m_mainMenuPrepState_1c8.base.vftable = kRecoilApp_MainMenuPrepState_VtblAddress;
    m_leaveNetworkState_1d0.base.vftable = kRecoilApp_LeaveNetworkState_VtblAddress;

    m_missionFmvState_1d8.Constructor();
    m_playState_208.Constructor();
    *(volatile RecoilPtr32 *)(&m_mpExitDialogState_220.base.vftable) =
        kRecoilApp_MpExitDialogState_VtblAddress;
    *(volatile RecoilPtr32 *)(&vftable) = kRecoilApp_VtblAddress;
    *(volatile unsigned int *)(&m_transitionFadeTimer150) = 0;
    return this;
}

// Reimplements 0x42de60: RecoilApp::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::Destructor() {
    m_mpExitDialogState_220.base.vftable = kRecoilStateBase_VtblAddress;
    m_playState_208.base.vftable = kRecoilStateBase_VtblAddress;

    ((zFMV_Script *)(m_missionFmvState_1d8.m_fmv_08))->Cleanup();
    m_missionFmvState_1d8.base.vftable = kRecoilStateBase_VtblAddress;
    m_leaveNetworkState_1d0.base.vftable = kRecoilStateBase_VtblAddress;
    m_mainMenuPrepState_1c8.base.vftable = kRecoilStateBase_VtblAddress;

    ((zFMV_Script *)(m_introFmvState_1a0.m_fmv_08))->Cleanup();
    m_introFmvState_1a0.base.vftable = kRecoilStateBase_VtblAddress;

    ((zFMV_Script *)(m_attractFmvState_160.m_fmv_10))->Cleanup();
    m_attractFmvState_160.base.vftable = kRecoilStateBase_VtblAddress;

    MfcOleModuleDestructor();
}

// Reimplements 0x42e0b0: RecoilApp::ScalarDeletingDestructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL
RecoilApp::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x443700: RecoilApp_StateQueueBlock::InitFromCursor
RecoilApp_StateQueueBlock *RECOIL_THISCALL
RecoilApp_StateQueueBlock::InitFromCursor(RecoilPtr32 cursor, RecoilPtr32 chunkPtrSlot) {
    // Original 0x443700 reloads through the chunk-slot pointer when deriving
    // m_chunkEnd; keep the volatile read so VC does not collapse the two loads.
    volatile RecoilApp_StateQueueBlock *const block = this;
    volatile RecoilPtr32 *const chunkSlot = (volatile RecoilPtr32 *)chunkPtrSlot;
    block->m_chunkBegin = *chunkSlot;
    const RecoilPtr32 chunkEnd = *chunkSlot + 0x1000;
    block->m_chunkPtrSlot = chunkPtrSlot;
    block->m_chunkEnd = chunkEnd;
    block->m_cursor = cursor;
    return this;
}

// Reimplements 0x443690: RecoilApp_StateQueue::GrowAndCenterChunkBaseList
RecoilPtr32 RECOIL_THISCALL
RecoilApp_StateQueue::GrowAndCenterChunkBaseList(int newCapacity) {
    int byteCount =
        (int)((unsigned int)(newCapacity) << 2);
    if (byteCount < 0) {
        byteCount = 0;
    }

    RecoilPtr32 *const newList = (RecoilPtr32 *)(::operator new((size_t)(byteCount)));
    RecoilPtr32 *const centerSlot = newList + ((unsigned int)(newCapacity) >> 2);

    const RecoilPtr32 * srcSlot = (const RecoilPtr32 *)(
        (unsigned int)(m_readBlock.m_chunkPtrSlot));
    const RecoilPtr32 *const srcEnd = (const RecoilPtr32 *)(
        (unsigned int)(m_writeBlock.m_chunkPtrSlot + 4));
    RecoilPtr32 *dstSlot = centerSlot;
    while (srcSlot != srcEnd) {
        *dstSlot = *srcSlot;
        ++srcSlot;
        ++dstSlot;
    }

    ::operator delete((void *)((unsigned int)(m_chunkPtrList)));
    m_chunkPtrList = (RecoilPtr32)((unsigned int)(newList));
    m_chunkPtrCapacity = newCapacity;
    return (RecoilPtr32)((unsigned int)(centerSlot));
}

// Reimplements 0x42de10: RecoilApp::GetMessageMap
const MfcMsgMap *RECOIL_THISCALL RecoilApp::GetMessageMap() const {
    return &g_RecoilApp_MessageMap;
}

// Reimplements 0x442c00: RecoilApp::GetMainWnd
RECOIL_NOINLINE RecoilPtr32 RECOIL_THISCALL RecoilApp::GetMainWnd() const {
    return m_pMainWnd;
}

// Reimplements 0x443140: RecoilApp::GetCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::GetCurrentState() const {
    if (m_currentStateIndex_0c8 < 0) {
        return 0;
    }

    if (m_currentStateIndex_0c8 >=
        (int)(sizeof(m_stateStack_0d8) / sizeof(m_stateStack_0d8[0]))) {
        return 0;
    }

    return m_stateStack_0d8[m_currentStateIndex_0c8];
}

// Reimplements 0x443160: RecoilApp::QueueSwitchCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueueSwitchCurrentState(RecoilApp_IState *state,
                                                               int stateParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    const RecoilPtr32 newState = (RecoilPtr32)((unsigned int)(state));

    RecoilApp_StateQueueItem *const item = (RecoilApp_StateQueueItem *)(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_SwitchCurrent;
        item->m_stateObj = newState;
        item->m_param = stateParam;
        itemValue = (RecoilPtr32)((unsigned int)(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            (RecoilPtr32)((unsigned int)(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = (RecoilPtr32 *)(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                (RecoilPtr32)((unsigned int)(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                (RecoilPtr32)((unsigned int)(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + (RecoilPtr32)(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    (int)(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + (RecoilPtr32)(activeChunkCount * 4);
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *(RecoilPtr32 *)((unsigned int)(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    if (currentState != 0) {
        CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl, OnExit));
    }
    CallRecoilStateMethod(newState, offsetof(RecoilApp_IState_Vtbl, OnEnter));

    return currentState;
}

// Reimplements 0x443310: RecoilApp::QueuePushState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueuePushState(RecoilApp_IState *state,
                                                      int suspendParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    const RecoilPtr32 newState = (RecoilPtr32)((unsigned int)(state));

    RecoilApp_StateQueueItem *const item = (RecoilApp_StateQueueItem *)(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_PushState;
        item->m_stateObj = newState;
        item->m_param = suspendParam;
        itemValue = (RecoilPtr32)((unsigned int)(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            (RecoilPtr32)((unsigned int)(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = (RecoilPtr32 *)(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                (RecoilPtr32)((unsigned int)(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                (RecoilPtr32)((unsigned int)(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + (RecoilPtr32)(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    (int)(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + (RecoilPtr32)(activeChunkCount * 4);
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *(RecoilPtr32 *)((unsigned int)(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    CallRecoilStateMethod(newState, offsetof(RecoilApp_IState_Vtbl, OnEnter));
    return currentState;
}

// Reimplements 0x4434b0: RecoilApp::QueueExitCurrentState
RecoilPtr32 RECOIL_THISCALL RecoilApp::QueueExitCurrentState(int stateParam) {
    const RecoilPtr32 currentState = GetCurrentState();

    RecoilApp_StateQueueItem *const item = (RecoilApp_StateQueueItem *)(::operator new(sizeof(RecoilApp_StateQueueItem)));
    RecoilPtr32 itemValue = 0;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_ExitCurrent;
        item->m_stateObj = 0;
        item->m_param = stateParam;
        itemValue = (RecoilPtr32)((unsigned int)(item));
    }

    RecoilApp_StateQueue &queue = m_stateQueue_118;
    if (queue.m_itemCount == 0 || queue.m_writeBlock.m_cursor == queue.m_writeBlock.m_chunkEnd) {
        const RecoilPtr32 chunk =
            (RecoilPtr32)((unsigned int)(::operator new(0x1000)));

        if (queue.m_itemCount == 0) {
            queue.m_chunkPtrCapacity = 2;
            RecoilPtr32 *const chunkList = (RecoilPtr32 *)(::operator new(2 * sizeof(RecoilPtr32)));
            queue.m_chunkPtrList =
                (RecoilPtr32)((unsigned int)(chunkList));
            chunkList[1] = chunk;

            const RecoilPtr32 chunkSlot =
                (RecoilPtr32)((unsigned int)(&chunkList[1]));
            const RecoilPtr32 centeredCursor = chunk + 0x800;
            queue.m_readBlock.InitFromCursor(centeredCursor, chunkSlot);
            queue.m_writeBlock.InitFromCursor(centeredCursor, chunkSlot);
        } else {
            RecoilPtr32 chunkSlot = queue.m_writeBlock.m_chunkPtrSlot;
            const RecoilPtr32 lastChunkSlot =
                queue.m_chunkPtrList + (RecoilPtr32)(queue.m_chunkPtrCapacity * 4 - 4);

            if (chunkSlot < lastChunkSlot) {
                chunkSlot += 4;
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            } else {
                const int activeChunkCount =
                    (int)(
                        (queue.m_writeBlock.m_chunkPtrSlot - queue.m_readBlock.m_chunkPtrSlot) /
                        4) +
                    1;
                const RecoilPtr32 newList = queue.GrowAndCenterChunkBaseList(activeChunkCount * 2);
                chunkSlot = newList + (RecoilPtr32)(activeChunkCount * 4);
                *(RecoilPtr32 *)((unsigned int)(chunkSlot)) = chunk;
                queue.m_readBlock.InitFromCursor(queue.m_readBlock.m_cursor, newList);
                queue.m_writeBlock.InitFromCursor(chunk, chunkSlot);
            }
        }
    }

    const RecoilPtr32 writeCursor = queue.m_writeBlock.m_cursor;
    queue.m_writeBlock.m_cursor = writeCursor + 4;
    if (writeCursor != 0) {
        *(RecoilPtr32 *)((unsigned int)(writeCursor)) = itemValue;
    }
    ++queue.m_itemCount;

    if (currentState != 0) {
        CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl, OnExit));
    }

    return currentState;
}

// Reimplements 0x442c10: RecoilApp::StartEngineAndQueueStartupState
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::StartEngineAndQueueStartupState() {
    const RecoilPtr32 appVtable = vftable;
    const RecoilPtr32 mainWnd = GetMainWnd();
    const RecoilPtr32 hwnd =
        *(const RecoilPtr32 *)((unsigned int)(mainWnd + 0x20));

    RecoilAppStartEngineMethod const startEngine = (RecoilAppStartEngineMethod)(
        (unsigned int)(ReadRecoilVtableSlot(appVtable, 0xac)));
    if (startEngine(this, hwnd) == 0) {
        RecoilAppNoArgVoidMethod const shutdownEngine = (RecoilAppNoArgVoidMethod)(
            (unsigned int)(ReadRecoilVtableSlot(vftable, 0xb0)));
        shutdownEngine(this);

        RecoilAppNoArgIntMethod const exitInstance = (RecoilAppNoArgIntMethod)(
            (unsigned int)(ReadRecoilVtableSlot(vftable, 0x70)));
        return exitInstance(this);
    }

    RecoilApp_IState *const startupState =
        (RecoilApp_IState *)((unsigned int)(m_pendingState_0c4));
    m_skipWait_0d0 = 1;
    m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    QueueSwitchCurrentState(startupState, 0);
    return 1;
}

// Reimplements 0x443650: RecoilApp::OnIdleOrDispatch
int RECOIL_THISCALL RecoilApp::OnIdleOrDispatch(unsigned int wParam,
                                                         unsigned int lParam) {
    const RecoilPtr32 currentState = GetCurrentState();
    zSndCd::OnMciNotify(wParam, lParam);
    if (currentState == 0) {
        return 0;
    }

    const RecoilFn32 methodValue = ReadRecoilVtableSlot(
        *(const RecoilPtr32 *)((unsigned int)(currentState)),
        offsetof(RecoilApp_IState_Vtbl, OnIdleOrDispatch));
    RecoilStateIdleMethod const method = (RecoilStateIdleMethod)((unsigned int)(methodValue));
    return method((RecoilApp_IState *)((unsigned int)(currentState)),
                  wParam, lParam);
}

// Reimplements 0x442a10: RecoilApp::TakeSkipWaitMessage
int RECOIL_THISCALL RecoilApp::TakeSkipWaitMessage() {
    const int wasSkipped = m_skipWait_0d0;
    m_skipWait_0d0 = 0;
    return wasSkipped;
}

// Reimplements 0x442a30: RecoilApp::MarkSkipWaitMessage
int RECOIL_THISCALL RecoilApp::MarkSkipWaitMessage() {
    const int wasSkipped = m_skipWait_0d0;
    m_skipWait_0d0 = 1;
    return wasSkipped;
}

// Reimplements 0x442c70: RecoilApp::MfcOleModuleConstructor
RECOIL_NOINLINE RecoilApp *RECOIL_THISCALL RecoilApp::MfcOleModuleConstructor() {
    new (this) CWinApp(0);

    const unsigned int selfValue = (unsigned int)(this);
    unsigned char *const ctorTagBytes = (unsigned char *)(&m_stateQueue_118.m_ctorTag_00);
    ctorTagBytes[0] = (unsigned char)((selfValue >> 24) & 0xff);

    memset(&m_stateQueue_118.m_readBlock, 0, 0x2c);
    m_skipWait_0d0 = 0;
    m_pendingState_0c4 = 0;
    vftable = kRecoilApp_MfcOleModule_VtblAddress;
    m_currentStateIndex_0c8 = -1;
    memset(m_stateStack_0d8, 0, sizeof(m_stateStack_0d8));
    return this;
}

// Reimplements 0x4428b0: RecoilApp::MfcOleModuleDestructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp::MfcOleModuleDestructor() {
    vftable = kRecoilApp_MfcOleModule_VtblAddress;

    while (m_stateQueue_118.m_itemCount != 0) {
        m_stateQueue_118.m_readBlock.m_cursor += 4;
        --m_stateQueue_118.m_itemCount;

        if (m_stateQueue_118.m_itemCount == 0 ||
            m_stateQueue_118.m_readBlock.m_cursor == m_stateQueue_118.m_readBlock.m_chunkEnd) {
            const RecoilPtr32 oldChunkSlot = m_stateQueue_118.m_readBlock.m_chunkPtrSlot;
            m_stateQueue_118.m_readBlock.m_chunkPtrSlot = oldChunkSlot + 4;

            const RecoilPtr32 chunk =
                *(RecoilPtr32 *)((unsigned int)(oldChunkSlot));
            ::operator delete((void *)((unsigned int)(chunk)));

            if (m_stateQueue_118.m_itemCount == 0) {
                memset(&m_stateQueue_118.m_readBlock, 0, sizeof(m_stateQueue_118.m_readBlock));
                memset(&m_stateQueue_118.m_writeBlock, 0,
                            sizeof(m_stateQueue_118.m_writeBlock));
                ::operator delete((void *)(
                    (unsigned int)(m_stateQueue_118.m_chunkPtrList)));
            } else {
                const RecoilPtr32 chunkSlot = m_stateQueue_118.m_readBlock.m_chunkPtrSlot;
                const RecoilPtr32 nextChunk =
                    *(RecoilPtr32 *)((unsigned int)(chunkSlot));
                m_stateQueue_118.m_readBlock.m_chunkBegin = nextChunk;
                m_stateQueue_118.m_readBlock.m_chunkEnd = nextChunk + 0x1000;
                m_stateQueue_118.m_readBlock.m_cursor = nextChunk;
                m_stateQueue_118.m_readBlock.m_chunkPtrSlot = chunkSlot;
            }
        }
    }

    ((CWinApp *)(this))->CWinApp::~CWinApp();
}

// Reimplements 0x4429b0: RecoilApp::MfcOleModuleScalarDeletingDestructor
RecoilApp *RECOIL_THISCALL RecoilApp::MfcOleModuleScalarDeletingDestructor(unsigned int flags) {
    MfcOleModuleDestructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x442d00: RecoilApp::Run
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp::Run()
{
    SetThreadPriority((HANDLE)((unsigned int)(m_mainThreadHandle_2c)),
                      THREAD_PRIORITY_HIGHEST);

    for (;;) {
        while (PeekMessageA((MSG *)(m_msg_34), 0, 0, 0, PM_NOREMOVE) != 0) {
            RecoilAppNoArgIntMethod const pumpMessage =
                (RecoilAppNoArgIntMethod)((unsigned int)(
                    ReadRecoilVtableSlot(vftable, 0x64)));
            if (pumpMessage(this) == 0) {
                RecoilAppNoArgIntMethod const exitInstance =
                    (RecoilAppNoArgIntMethod)((unsigned int)(
                        ReadRecoilVtableSlot(vftable, 0x70)));
                return exitInstance(this);
            }
        }

        zNetworkDPlay::ReceivePendingMessages(-1);

        const RecoilPtr32 currentState = GetCurrentState();
        if (m_skipWait_0d0 == 0) {
            if (PeekMessageA((MSG *)(m_msg_34), 0, 0, 0, PM_NOREMOVE) == 0) {
                WaitMessage();
            }
            continue;
        }

        RecoilApp_StateQueue &queue = m_stateQueue_118;
        if (queue.m_itemCount != 0) {
            RecoilPtr32 *const queueSlot =
                (RecoilPtr32 *)((unsigned int)(queue.m_readBlock.m_cursor));
            RecoilApp_StateQueueItem *const item =
                (RecoilApp_StateQueueItem *)((unsigned int)(*queueSlot));

            if (item->m_kind == RecoilApp_StateQueueKind_ExitCurrent) {
                if (currentState != 0) {
                    CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl,
                                                                 OnDeactivate));
                }

                m_stateStack_0d8[m_currentStateIndex_0c8] = 0;
                --m_currentStateIndex_0c8;
                if (m_currentStateIndex_0c8 < 0) {
                    m_currentStateIndex_0c8 = 0;
                }

                CallRecoilStateParamMethod(m_stateStack_0d8[m_currentStateIndex_0c8],
                                           offsetof(RecoilApp_IState_Vtbl, OnResume),
                                           item->m_param);
            } else if (item->m_kind == RecoilApp_StateQueueKind_PushState) {
                if (item->m_stateObj != 0) {
                    CallRecoilStateParamMethod(m_stateStack_0d8[m_currentStateIndex_0c8],
                                               offsetof(RecoilApp_IState_Vtbl, OnSuspend),
                                               item->m_param);

                    if (CallRecoilStateIntMethod(item->m_stateObj,
                                                 offsetof(RecoilApp_IState_Vtbl,
                                                          OnCanBecomeCurrent)) != 0) {
                        ++m_currentStateIndex_0c8;
                        if (m_currentStateIndex_0c8 >= 16) {
                            m_currentStateIndex_0c8 = 15;
                        }

                        m_stateStack_0d8[m_currentStateIndex_0c8] = item->m_stateObj;
                    }
                }
            } else if (item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent) {
                if (item->m_stateObj != 0) {
                    if (currentState != 0) {
                        CallRecoilStateMethod(currentState, offsetof(RecoilApp_IState_Vtbl,
                                                                     OnDeactivate));
                    }

                    if (m_currentStateIndex_0c8 < 0) {
                        m_currentStateIndex_0c8 = 0;
                    }
                    if (m_currentStateIndex_0c8 >= 16) {
                        m_currentStateIndex_0c8 = 15;
                    }

                    if (CallRecoilStateIntMethod(item->m_stateObj,
                                                 offsetof(RecoilApp_IState_Vtbl,
                                                          OnCanBecomeCurrent)) != 0) {
                        m_stateStack_0d8[m_currentStateIndex_0c8] = item->m_stateObj;
                    } else if (currentState != 0) {
                        CallRecoilStateIntMethod(currentState,
                                                 offsetof(RecoilApp_IState_Vtbl,
                                                          OnCanBecomeCurrent));
                    }
                }
            }

            queue.m_readBlock.m_cursor += 4;
            --queue.m_itemCount;
            if (queue.m_itemCount == 0 ||
                queue.m_readBlock.m_cursor == queue.m_readBlock.m_chunkEnd) {
                const RecoilPtr32 oldChunkSlot = queue.m_readBlock.m_chunkPtrSlot;
                queue.m_readBlock.m_chunkPtrSlot = oldChunkSlot + 4;

                const RecoilPtr32 oldChunk =
                    *(RecoilPtr32 *)((unsigned int)(oldChunkSlot));
                ::operator delete((void *)((unsigned int)(oldChunk)));

                if (queue.m_itemCount == 0) {
                    memset(&queue.m_readBlock, 0, sizeof(queue.m_readBlock));
                    memset(&queue.m_writeBlock, 0, sizeof(queue.m_writeBlock));
                    ::operator delete((void *)((unsigned int)(queue.m_chunkPtrList)));
                } else {
                    const RecoilPtr32 chunkSlot = queue.m_readBlock.m_chunkPtrSlot;
                    const RecoilPtr32 nextChunk =
                        *(RecoilPtr32 *)((unsigned int)(chunkSlot));
                    queue.m_readBlock.m_chunkBegin = nextChunk;
                    queue.m_readBlock.m_chunkEnd = nextChunk + 0x1000;
                    queue.m_readBlock.m_cursor = nextChunk;
                    queue.m_readBlock.m_chunkPtrSlot = chunkSlot;
                }
            }

            ::operator delete(item);
            continue;
        }

        if (currentState != 0 &&
            CallRecoilStateIntMethod(currentState, offsetof(RecoilApp_IState_Vtbl,
                                                            OnUpdateShouldQuit)) != 0) {
            RecoilAppNoArgVoidMethod const onAppDeactivate =
                (RecoilAppNoArgVoidMethod)((unsigned int)(
                    ReadRecoilVtableSlot(vftable, 0xa8)));
            onAppDeactivate(this);
            PostQuitMessage(0);
        }
    }
}

// Reimplements 0x42eea0: RecoilApp_PlayState::Constructor
RECOIL_NOINLINE RecoilApp_PlayState *RECOIL_THISCALL RecoilApp_PlayState::Constructor() {
    base.vftable = kRecoilApp_PlayState_VtblAddress;
    m_transitionScratch_10 = 0;
    pPendingLoadGameStartPath = 0;
    return this;
}

// Reimplements 0x42eec0: RecoilApp_PlayState::OnWndActivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_PlayState::OnWndActivate(int bActivate) {
    if (bActivate != 0) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }
}

// Reimplements 0x42eed0: RecoilApp_PlayState::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_PlayState::OnTryBecomeCurrent() {
    const int completedObjectiveCount = g_HudSensorTracker.completedObjectiveCount;

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(SPI_SETSCREENSAVERRUNNING, 1, &screenSaverRunning, 0);
    }

    Time::Reset();
    g_FrameDeltaTimeSec = 0.100000001f;

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiNetExitPanel::CreateGlobal();
    }

    int effectsLevel = zOpt::GetEffectsLevelForCurrentHwMode();
    if (zVid::GetAccelerationOption() == 0 && effectsLevel == 0) {
        effectsLevel = 1;
    }
    zOpt::SetEffectsLevelForCurrentHwMode(effectsLevel);

    HudUiMgr::EnsureHudLoaded("hud.zrd");
    HudUiLoadingCheckpoint::InitTable();
    HudUiLoadingCheckpoint::AdvanceAndLog("Loading common sounds");
    zSndSampleSet_InitByName("COMMON");

    Briefing::StartForMission(g_HudSensorTracker.GetMissionId());

    char loadingMessage[0x100];
    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 3,
                        RecoilVersion::GetString());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 5,
                        zVid::GetSelectedHwApiDescriptionOrDefault());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(loadingMessage, sizeof(loadingMessage), 6,
                        zVid::GetSelectedD3DDeviceNameOrDefault());
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10d));

    g_HudSensorTracker.LoadObjectivesFromPath("objectives.zrd");
    Player::ZAR_RegisterSections();
    Briefing::BuildObjectiveActionsGlobal(completedObjectiveCount);

    if (g_HudSensorTracker.LoadMissionCoreResources() == 0) {
        return 0;
    }

    g_HudSensorTracker.InitMissionGameplaySystems();
    Briefing::StopAndShutdownThread(1);
    HudUiMgr::ApplyHudModeSwitch(ZOPT_HUD_TYPE_STANDARD);

    const char *startAnimNodeName;
    if (pPendingLoadGameStartPath != 0) {
        ExtendPlayStateTransitionTimer(5.0f);

        char *const pendingLoadPath = (char *)((unsigned int)(
            pPendingLoadGameStartPath));
        zUtil::ZAR_LoadFileGlobal(pendingLoadPath);
        free(pendingLoadPath);
        pPendingLoadGameStartPath = 0;
        startAnimNodeName = "LOAD_GAME_START";
    } else {
        startAnimNodeName = "NEW_GAME_START";
    }

    g_HudSensorTracker.RunStartAnimsFromZrd("StartAnims.zrd", startAnimNodeName);

    pRenderSection = (RecoilPtr32)(zOpt::GetRenderSection());
    pDisplaySection = (RecoilPtr32)(zOpt::GetDisplaySection());
    pWindowSection = (RecoilPtr32)(zOpt::GetWindowSection());

    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    if (zVid::GetAccelerationOption() != 0) {
        zClass_Camera::SetActiveCamera(0);
        zClass_Camera::SetObjectHseTestEnabled(0);
    }

    ExtendPlayStateTransitionTimer(1.0f);

    TickAndRenderFrame(0);
    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    g_zVideo_FrameTick = 0;
    g_RecoilApp.m_reserved148 = 1;
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_ENABLED);
    g_HudSensorTracker.ResetHudForMissionStart();

    if (zInput::Mouse_IsInitialized() != 0) {
        g_zInput_MouseActive = 0;
        zInput::Mouse_UpdateAcquireState();
    }

    zInput::ResetAllTransitionState();

    zOpt::SetGraphicsFlagsForCurrentHwMode(zOpt::GetGraphicsFlagsForCurrentHwMode());
    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode((missionId % (trackCount - 2)) + 2, 5);
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            ExtendPlayStateTransitionTimer(5.0f);
            HudUiMgr::EnableTopAndChatStacks();
            return 1;
        }

        HudUiMgr::EnableTopAndChatStacks();
    }

    return 1;
}

// Reimplements 0x42f5e0: RecoilApp_PlayState::OnUpdateShouldQuit
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_PlayState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_transitionFadeTimer150 > 0.0f) {
        g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED;
        TickAndRenderFrame(0);

        zOpt_ViewRectSection *const windowSection = ViewRectFromPtr(pWindowSection);
        if (g_RecoilApp.m_transitionFadeTimer150 >= 1.0f) {
            const int previousClearState =
                zVideo::ExchangeClearScreenBufferEnabled(ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED);
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 1;
            if (zVid::GetAccelerationOption() != 0) {
                zVideo::CallClearSwSurfaceAndZBuffer((zVidRect32 *)windowSection,
                                                     (zVidRect32 *)windowSection);
            } else {
                zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32 *)windowSection);
            }
            zVideo::ExchangeClearScreenBufferEnabled(previousClearState);
        } else {
            const double overlayAlpha =
                g_RecoilApp.m_transitionFadeTimer150 > 0.0f
                    ? (double)(g_RecoilApp.m_transitionFadeTimer150)
                    : 0.0;
            zRndr_OverlayRect_Submit(0, 0, overlayAlpha);
        }

        zVideo::AdjustSurfacesIfEnabled((zVidRect32 *)windowSection, (zVidRect32 *)windowSection,
                                        0, 0);
        g_RecoilApp.m_transitionFadeTimer150 -= g_FrameDeltaTimeSec;

        if (g_RecoilApp.m_transitionFadeTimer150 <= 0.0f) {
            zOpt::SetMuteSoundOption(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 0;
        }

        return 0;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        zSndPlayHandleSnapshot *const snapshot =
            zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
        zSndCd::Stop();

        zFMV_Script fmvScript;
        fmvScript.Init("fmv.zrd", "GRANDPRIZE", 0);
        fmvScript.RunBlocking(0);

        if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
        }

        zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);

        RunGrandPrizeBlurAction();
        fmvScript.Cleanup();
        return 0;
    }

    g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED;
    if (TickAndRenderFrame(1) != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Show();
            return 0;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)ViewRectFromPtr(pWindowSection));
        if (g_RecoilApp_QuitAfterCredits == 0) {
            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
        }
    }

    return 0;
}

// Reimplements 0x42f8a0: RecoilApp_PlayState::OnResume
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_PlayState::OnResume(int) {
    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode((missionId % (trackCount - 2)) + 2, 5);
    }
}

// Reimplements 0x42f8e0: RecoilApp_PlayState::OnDeactivate
// (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_PlayState::OnDeactivate() {
    HudUiLoadingCheckpoint::AdvanceAndLog("Leaving Play State");

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(SPI_SETSCREENSAVERRUNNING, 0, &screenSaverRunning, 0);
    }

    zSndCd::Stop();
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog("Leaving Networking");
        HudUiNetExitPanel::DestroyGlobal();
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog("Stop All Sounds");
        zSndPlayHandleSnapshot *const snapshot =
            zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
    }

    zFMV_Script fmvScript;
    fmvScript.Init("fmv.zrd", "MISSIONOVER", 0);
    fmvScript.RunBlocking(1);

    if (g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_ON_EXIT) {
        g_HudSensorTracker.ShutdownMissionGameplaySystems();
    }

    zUtil_ZRDR_UnloadMountedArchives(0);
    fmvScript.Cleanup();
}

// Reimplements 0x42f9d0: RecoilApp_LeaveNetworkState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_LeaveNetworkState::OnTryBecomeCurrent() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    g_RecoilApp.ShutdownEngine();
    zSndBackend::Shutdown();
    return 1;
}

// Reimplements 0x42eb70: RecoilApp_AttractFmvState::Constructor
RECOIL_NOINLINE RecoilApp_AttractFmvState *RECOIL_THISCALL
RecoilApp_AttractFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    ((zFMV_Script *)(m_fmv_10))->Init(0, 0, 0);
    base.vftable = kRecoilApp_AttractFmvState_VtblAddress;
    return this;
}

RECOIL_NOINLINE RecoilApp_IntroFmvState *RECOIL_THISCALL RecoilApp_IntroFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    ((zFMV_Script *)(m_fmv_08))->Init(0, 0, 0);
    base.vftable = kRecoilApp_IntroFmvState_VtblAddress;
    return this;
}

// Reimplements 0x42ea20: RecoilApp_IntroFmvState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_IntroFmvState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    zRndr::SetFrameBufferRegion(zVideo::GetPrimarySurfacePixels(), windowSection, bitsPerPixel,
                                pitchBytes);
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());

    const int fxPitchBytes = zVideo::GetPrimarySurfacePitch();
    const int surfaceHeight = zVideo::GetPrimarySurfaceHeight();
    const int surfaceWidth = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), surfaceWidth, surfaceHeight,
                               fxPitchBytes);
    zVid::SetCachedClientRectUpdateMask(1);

    if (g_RecoilApp.m_skipIntroFmv == 0) {
        zFMV_Script *const script = (zFMV_Script *)(m_fmv_08);
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd("fmv.zrd", "INTRO") != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

// Reimplements 0x42eac0: RecoilApp_IntroFmvState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_IntroFmvState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_skipIntroFmv != 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState_1d8.base, 0);
        return 0;
    }

    zFMV_Script *const script = (zFMV_Script *)(m_fmv_08);
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState_1c8.base, stateParam);
    }

    return 0;
}

// Reimplements 0x42eb00: RecoilApp_FmvState::OnIdleOrDispatch
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_FmvState::OnIdleOrDispatch(unsigned int,
                                                                                  unsigned int) {
    return 1;
}

// Reimplements 0x42eb10: RecoilApp_IntroFmvState::OnDeactivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IntroFmvState::OnDeactivate() {
    ((zFMV_Script *)(m_fmv_08))->BeginNow(1);
}

// Reimplements 0x42eb20: RecoilApp_MainMenuPrepState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MainMenuPrepState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int height = zVideo::GetPrimarySurfaceHeight();
    const int width = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), width, height, pitchBytes);
    m_stateData04 = 0;
    return 1;
}

// Reimplements 0x42eb60: RecoilApp_MainMenuPrepState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MainMenuPrepState::OnUpdateShouldQuit() {
    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    return 0;
}

// Reimplements 0x42ebf0: RecoilApp_AttractFmvState::OnTryBecomeCurrent
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_AttractFmvState::OnTryBecomeCurrent() {
    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int height = zVideo::GetPrimarySurfaceHeight();
    const int width = zVideo::GetPrimarySurfaceWidth();
    zVideo::Fx_SetSurfaceState(zVideo::GetPrimarySurfacePixels(), width, height, pitchBytes);

    GetClientRect(g_RecoilApp_hWndMain, (RECT *)(m_clientRect_30));

    zFMV_Script *const script = (zFMV_Script *)(m_fmv_10);
    if (g_RecoilApp_AttractFmvReloadMode != 0) {
        script->LoadActionsFromZrd("fmv.zrd", "ATTRACT");
        g_RecoilApp_AttractFmvReloadMode = 0;
    }

    if (g_RecoilApp_hWndMain != 0) {
        script->m_hWnd = g_RecoilApp_hWndMain;
    }

    if (script->LoadActionsFromZrd("fmv.zrd", "ATTRACT") != -1) {
        script->BeginAtTime();
    }

    return 1;
}

// Reimplements 0x42ec80: RecoilApp_AttractFmvState::OnUpdateShouldQuit
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_AttractFmvState::OnUpdateShouldQuit() {
    zFMV_Script *const script = (zFMV_Script *)(m_fmv_10);
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_mainMenuPrepState_1c8.base, stateParam);
    }

    return 0;
}

// Reimplements 0x42eca0: RecoilApp_AttractFmvState::OnDeactivate
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_AttractFmvState::OnDeactivate() {
    ((zFMV_Script *)(m_fmv_10))->BeginNow(0);
}

// Reimplements 0x42ed30: RecoilApp_MissionFmvState::Constructor
RECOIL_NOINLINE RecoilApp_MissionFmvState *RECOIL_THISCALL
RecoilApp_MissionFmvState::Constructor() {
    base.vftable = kRecoilStateBase_VtblAddress;
    ((zFMV_Script *)(m_fmv_08))->Init(0, 0, 0);
    base.vftable = kRecoilApp_MissionFmvState_VtblAddress;
    m_missionId = 0;
    m_skipMissionFmv = 0;
    return this;
}

// Reimplements 0x42df90: RecoilApp_IState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IState::Destructor() {
    vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42e0f0: RecoilApp_IState::ScalarDeletingDestructor
RecoilApp_IState *RECOIL_THISCALL RecoilApp_IState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42df10: RecoilApp_AttractFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_AttractFmvState::Destructor() {
    ((zFMV_Script *)(m_fmv_10))->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42df50: RecoilApp_IntroFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_IntroFmvState::Destructor() {
    ((zFMV_Script *)(m_fmv_08))->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42ebd0: RecoilApp_AttractFmvState::ScalarDeletingDestructor
RecoilApp_AttractFmvState *RECOIL_THISCALL
RecoilApp_AttractFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42e0d0: RecoilApp_IntroFmvState::ScalarDeletingDestructor
RecoilApp_IntroFmvState *RECOIL_THISCALL
RecoilApp_IntroFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x42e070: RecoilApp_MissionFmvState::Destructor
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_MissionFmvState::Destructor() {
    ((zFMV_Script *)(m_fmv_08))->Cleanup();
    base.vftable = kRecoilStateBase_VtblAddress;
}

// Reimplements 0x42ed90: RecoilApp_MissionFmvState::ScalarDeletingDestructor
RecoilApp_MissionFmvState *RECOIL_THISCALL
RecoilApp_MissionFmvState::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}
