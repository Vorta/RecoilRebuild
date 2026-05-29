#include "Battlesport/CZGameFrame.h"

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zVideo/zVideo.h"


HINSTANCE RECOIL_STDCALL AfxFindResourceHandle(LPCSTR resourceName, LPCSTR resourceType);

namespace zSndCd {
int RECOIL_CDECL Stop();
}

class CPaintDC {
  public:
    CPaintDC(CWnd *pWnd);
    virtual ~CPaintDC();

    HDC m_hDC;
    HDC m_hAttribDC;
    HWND m_hWnd;
    BOOL m_bPrinting;
    PAINTSTRUCT m_ps;
};

RECOIL_STATIC_ASSERT(offsetof(CPaintDC, m_hDC) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(CPaintDC, m_ps) == 0x14);

namespace {
const RecoilNamedVtable kCZGameFrame_Vtable = {"CZGameFrame vtable"};
const RecoilNamedVtable kCBitmap_Vtable = {"CBitmap vtable"};
const RecoilNamedVtable kCGdiObject_Vtable = {"CGdiObject vtable"};
const RecoilNamedVtable kCObject_Vtable = {"CObject vtable"};

typedef void (RECOIL_THISCALL *RecoilStateWndActivateMethod)(RecoilApp_IState *, unsigned int);
typedef void (RECOIL_THISCALL *CFrameWndDestructorProc)(CFrameWnd *);

RecoilPtr32 Ptr32FromSymbol(const void *symbol) {
    return (RecoilPtr32)((unsigned int)(symbol));
}

CRuntimeClass *RECOIL_STDCALL GetCZGameFrameBaseRuntimeClass() {
    return (CRuntimeClass *)(&CFrameWnd::classCFrameWnd);
}

void CallStateWndActivate(RecoilPtr32 stateValue, unsigned int nState) {
    RecoilApp_IState *const state = (RecoilApp_IState *)((unsigned int)(stateValue));
    const RecoilApp_IState_Vtbl *const vtable = (const RecoilApp_IState_Vtbl *)(
        (unsigned int)(state->vftable));
    RecoilStateWndActivateMethod const method = (RecoilStateWndActivateMethod)(
        (unsigned int)(vtable->OnWndActivate));
    method(state, nState);
}

void CallMfcCFrameWndDestructor(CFrameWnd *frame) {
    HMODULE mfc42 = GetModuleHandleA("MFC42.DLL");
    if (mfc42 == 0) {
        mfc42 = LoadLibraryA("MFC42.DLL");
    }

    if (mfc42 != 0) {
        CFrameWndDestructorProc const destructor = (CFrameWndDestructorProc)(
            GetProcAddress(mfc42, "??1CFrameWnd@@UAE@XZ"));
        if (destructor != 0) {
            destructor(frame);
        }
    }
}
} // namespace

AFX_MSGMAP_ENTRY const CZGameFrame::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP CZGameFrame::messageMap = {
    &CZGameFrame::GetBaseMessageMapForMfc,
    &CZGameFrame::messageEntries[0],
};

CRuntimeClass CZGameFrame::classCZGameFrame = {
    "CZGameFrame", sizeof(CZGameFrame), 0xffff, 0, &GetCZGameFrameBaseRuntimeClass, 0,
};

// Reimplements 0x443790: CZGameFrame::GetBaseRuntimeClass
RECOIL_GAME_FRAME_NOINLINE RecoilPtr32 RECOIL_CDECL CZGameFrame::GetBaseRuntimeClass() {
    return Ptr32FromSymbol(&CFrameWnd::classCFrameWnd);
}

const AFX_MSGMAP *RECOIL_STDCALL CZGameFrame::GetBaseMessageMapForMfc() {
    return &CFrameWnd::messageMap;
}

// Reimplements 0x443730: CZGameFrame::CreateObject
RECOIL_GAME_FRAME_NOINLINE CZGameFrame *RECOIL_CDECL CZGameFrame::CreateObject() {
    CZGameFrame *const frame = (CZGameFrame *)(::operator new(sizeof(CZGameFrame)));
    if (frame == 0) {
        return 0;
    }

    try {
        return frame->Constructor("gamez");
    } catch (...) {
        ::operator delete(frame);
        throw;
    }
}

// Reimplements 0x4437a0: CZGameFrame::GetRuntimeClass
RECOIL_GAME_FRAME_NOINLINE RecoilPtr32 RECOIL_CDECL CZGameFrame::GetRuntimeClass() {
    return Ptr32FromSymbol(&CZGameFrame::classCZGameFrame);
}

// Reimplements 0x4437b0: CZGameFrame::GetBaseMessageMap
RECOIL_GAME_FRAME_NOINLINE RecoilPtr32 RECOIL_CDECL CZGameFrame::GetBaseMessageMap() {
    return Ptr32FromSymbol(&CFrameWnd::messageMap);
}

// Reimplements 0x4437c0: CZGameFrame::GetMessageMap
RECOIL_GAME_FRAME_NOINLINE RecoilPtr32 RECOIL_CDECL CZGameFrame::GetMessageMap() {
    return Ptr32FromSymbol(&CZGameFrame::messageMap);
}

// Reimplements 0x4438a0: CZGameFrame::IsWindowValid
RECOIL_GAME_FRAME_NOINLINE int RECOIL_STDCALL
CZGameFrame::IsWindowValid(CZGameFrameMfcWindow *pWnd) {
    if (pWnd == 0) {
        return 0;
    }

    typedef int (RECOIL_THISCALL *QueryValidityMethod)(CZGameFrameMfcWindow *);
    QueryValidityMethod const method = (QueryValidityMethod)(pWnd->vftable[4]);
    return method(pWnd) == 0 ? 1 : 0;
}

// Reimplements 0x4437d0: CZGameFrame::Constructor
RECOIL_GAME_FRAME_NOINLINE CZGameFrame *RECOIL_THISCALL
CZGameFrame::Constructor(const char *appId) {
    new ((CFrameWnd *)(this)) CFrameWnd();
    m_gameBitmap.vftable = (RecoilNamedVtable *)(&kCBitmap_Vtable);
    m_gameBitmap.m_hObject = 0;
    *(RecoilPtr32 *)(this) = Ptr32FromSymbol(&kCZGameFrame_Vtable);
    RecoilApp::InitStdLogFiles(appId);
    zVideo::ModuleInit();
    return this;
}

// Reimplements 0x443830: CZGameFrame::Destructor
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::Destructor() {
    *(RecoilPtr32 *)(this) = Ptr32FromSymbol(&kCZGameFrame_Vtable);
    zVideo::ReturnSuccessStub();
    m_gameBitmap.vftable = (RecoilNamedVtable *)(&kCGdiObject_Vtable);
    m_gameBitmap.DeleteObject();
    m_gameBitmap.vftable = (RecoilNamedVtable *)(&kCObject_Vtable);
    CallMfcCFrameWndDestructor((CFrameWnd *)(this));
}

// Reimplements 0x4438c0: CZGameFrame::BuildWindowTitle
RECOIL_GAME_FRAME_NOINLINE CString *RECOIL_THISCALL
CZGameFrame::BuildWindowTitle(CString *outTitle) {
    new (outTitle) CString("Zipper Interactive");
    return outTitle;
}

// Reimplements 0x443a60: CZGameFrame::OnCreate
RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL
CZGameFrame::OnCreate(CREATESTRUCTA *createStruct) {
    const int result = CFrameWnd::OnCreate(createStruct);
    if (result == -1) {
        return result;
    }

    m_gameBitmap.Attach(
        LoadBitmapA(AfxFindResourceHandle("GAMEBMP", MAKEINTRESOURCEA(2)), "GAMEBMP"));
    zInput::Mouse_ShutdownDevice();
    return 0;
}

// Reimplements 0x4438f0: CZGameFrame::OnClose
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::OnClose() {
    CFrameWnd::OnClose();
}

// Reimplements 0x443900: CZGameFrame::OnPaint
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::OnPaint() {
    CPaintDC paintDc((CWnd *)(void *)this);
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        return;
    }

    PAINTSTRUCT paintStruct = paintDc.m_ps;
    HDC compatibleDc = CreateCompatibleDC(paintDc.m_hDC);
    SelectObject(compatibleDc, m_gameBitmap.m_hObject);

    const int paintLeft = paintStruct.rcPaint.left;
    const int paintTop = paintStruct.rcPaint.top;
    const int paintWidth = paintStruct.rcPaint.right - paintLeft;
    const int paintHeight = paintStruct.rcPaint.bottom - paintTop;
    if (paintHeight > 480) {
        StretchBlt(paintDc.m_hDC, paintLeft, paintTop, paintWidth, paintHeight, compatibleDc,
                   paintLeft, paintTop, 640, 480, SRCCOPY);
    } else {
        BitBlt(paintDc.m_hDC, paintLeft, paintTop, paintWidth, paintHeight, compatibleDc,
               paintLeft, paintTop, SRCCOPY);
    }

    DeleteDC(compatibleDc);
}

// Reimplements 0x443ab0: CZGameFrame::OnDestroy
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::OnDestroy() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    zVideo::ShutdownVideoSystem();
    zSndCd::Stop();
    CFrameWnd::OnDestroy();
    m_gameBitmap.DeleteObject();
}

// Reimplements 0x443ae0: CZGameFrame::OnActivate
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::OnActivate(unsigned int nState,
                                                                        CWnd *pWndOther,
                                                                        BOOL bMinimized) {
    CFrameWnd::OnActivate(nState, pWndOther, bMinimized);

    RecoilApp *const app = (RecoilApp *)(m_app);
    const RecoilPtr32 currentState = app->GetCurrentState();
    if (currentState != 0) {
        CallStateWndActivate(currentState, nState);
    }

    if (nState == 0) {
        m_app->vftable->OnAppDeactivate(m_app);
        zInput::OnAppDeactivate();
        zGame::ReturnOnlyStub();
    } else {
        m_app->vftable->OnAppActivate(m_app);
        zInput::OnAppActivate();
        zVideo_RestoreIconicFullscreenWindowIfNeeded();
    }
}

// Reimplements 0x443a20: CZGameFrame::OnSize
RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL CZGameFrame::OnSize(unsigned int nType,
                                                                    int cx,
                                                                    int cy) {
    CFrameWnd::OnSize(nType, cx, cy);
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
}

// Reimplements 0x443b50: CZGameFrame::OnAppIdleDispatchMessage
RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL
CZGameFrame::OnAppIdleDispatchMessage(unsigned int wParam, unsigned int lParam) {
    return m_app->vftable->OnIdleOrDispatch(m_app, wParam, lParam);
}
