#include "Battlesport/CZGameFrame.h"

#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zVideo/zVideo.h"

HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

namespace zSndCd {
int Stop();
}

RECOIL_STATIC_ASSERT(
    offsetof(
        CPaintDC,
        m_hDC
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CPaintDC,
        m_ps
    ) == 0x14
);

namespace {
typedef void( *RecoilStateWndActivateMethod)(
    RecoilApp_IState *,
    unsigned int
);
typedef void( *CFrameWndDestructorProc)(CFrameWnd *);
typedef CObject *(PASCAL *MfcCreateObjectProc)();
typedef CRuntimeClass *(PASCAL *MfcRuntimeClassProc)();
typedef const AFX_MSGMAP *(PASCAL *MfcMessageMapProc)();

const UINT kMfcMessageMapSigIntCreateStruct = 9;
const UINT kMfcMessageMapSigLongWparamLparam = 10;
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidIntInt = 15;
const UINT kMfcMessageMapSigVoidUIntIntInt = 17;
const UINT kMfcMessageMapSigVoidUIntCWndBool = 28;

/**
 * Local MFC provider boundary ABI helper for CZGameFrame teardown.
 *
 * Purpose: resolve and call the MFC42 CFrameWnd destructor without rebuilding
 * provider-owned MFC internals in production source.
 */
void CallMfcCFrameWndDestructor(
    CFrameWnd *frame
) {
    HMODULE mfc42 = GetModuleHandleA("MFC42.DLL");
    if (mfc42 == 0) {
        mfc42 = LoadLibraryA("MFC42.DLL");
    }

    if (mfc42 != 0) {
        CFrameWndDestructorProc const destructor =
            (CFrameWndDestructorProc)(GetProcAddress(
                mfc42,
                "??1CFrameWnd@@UAE@XZ"
            ));
        if (destructor != 0) {
            destructor(frame);
        }
    }
}
} // namespace

/**
 * Reimplements data 0x4d2100: g_CZGameFrame_MessageEntries.
 *
 * Purpose: provide the terminal MFC message-map entry array for CZGameFrame.
 */
AFX_MSGMAP_ENTRY const CZGameFrame::messageEntries[] = {
    {WM_CLOSE, 0, 0, 0, kMfcMessageMapSigVoid, (AFX_PMSG)&CZGameFrame::OnClose},
    {WM_PAINT, 0, 0, 0, kMfcMessageMapSigVoid, (AFX_PMSG)&CZGameFrame::OnPaint},
    {WM_SIZE, 0, 0, 0, kMfcMessageMapSigVoidUIntIntInt, (AFX_PMSG)&CZGameFrame::OnSize},
    {WM_MOVE, 0, 0, 0, kMfcMessageMapSigVoidIntInt, (AFX_PMSG)&CZGameFrame::OnMove},
    {WM_CREATE, 0, 0, 0, kMfcMessageMapSigIntCreateStruct, (AFX_PMSG)&CZGameFrame::OnCreate},
    {WM_DESTROY, 0, 0, 0, kMfcMessageMapSigVoid, (AFX_PMSG)&CZGameFrame::OnDestroy},
    {0x3b9,
        0,
        0,
        0,
        kMfcMessageMapSigLongWparamLparam,
        (AFX_PMSG)&CZGameFrame::OnAppIdleDispatchMessage},
    {WM_ACTIVATE,
        0,
        0,
        0,
        kMfcMessageMapSigVoidUIntCWndBool,
        (AFX_PMSG)&CZGameFrame::OnActivate},
    {0, 0, 0, 0, 0, 0},
};

/**
 * Reimplements data 0x4d20f8: g_CZGameFrame_MessageMap.
 *
 * Purpose: link CZGameFrame's message entries to the CFrameWnd provider
 * message-map accessor used as the retail base-map callback.
 */
const AFX_MSGMAP CZGameFrame::messageMap = {
    (MfcMessageMapProc)&CZGameFrame::GetBaseMessageMap,
    &CZGameFrame::messageEntries[0],
};

/**
 * Reimplements data 0x4d20e0: g_CZGameFrame_RuntimeClass.
 *
 * Purpose: expose CZGameFrame's MFC runtime-class record with the recovered
 * factory and CFrameWnd base-runtime callback pointer identities.
 */
CRuntimeClass CZGameFrame::classCZGameFrame = {
    "CZGameFrame",
    sizeof(CZGameFrame),
    0xffff,
    (MfcCreateObjectProc)&CZGameFrame::CreateObject,
    (MfcRuntimeClassProc)&CZGameFrame::GetBaseRuntimeClass,
    0,
};

/**
 * Reimplements 0x443790: CZGameFrame::GetBaseRuntimeClass.
 *
 * Purpose: return the MFC CFrameWnd runtime-class symbol for CZGameFrame's
 * recovered runtime-class hierarchy.
 */
RecoilPtr32 CZGameFrame::GetBaseRuntimeClass() {
    return (RecoilPtr32)((unsigned int)(&CFrameWnd::classCFrameWnd));
}

/**
 * Reimplements 0x443730: CZGameFrame::CreateObject.
 *
 * Purpose: allocate and construct the game frame object for the recovered MFC
 * runtime-class factory path.
 */
CZGameFrame *CZGameFrame::CreateObject() {
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

/**
 * Reimplements 0x4437a0: CZGameFrame::GetRuntimeClass.
 *
 * Purpose: return CZGameFrame's recovered runtime-class record to MFC callers.
 */
RecoilPtr32 CZGameFrame::GetRuntimeClass() {
    return (RecoilPtr32)((unsigned int)(&CZGameFrame::classCZGameFrame));
}

/**
 * Reimplements 0x4437b0: CZGameFrame::GetBaseMessageMap.
 *
 * Purpose: return the provider CFrameWnd message-map symbol for the frame's
 * recovered message-map hierarchy.
 */
RecoilPtr32 CZGameFrame::GetBaseMessageMap() {
    return (RecoilPtr32)((unsigned int)(&CFrameWnd::messageMap));
}

/**
 * Reimplements 0x4437c0: CZGameFrame::GetMessageMap.
 *
 * Purpose: return CZGameFrame's recovered MFC message-map record.
 */
RecoilPtr32 CZGameFrame::GetMessageMap() {
    return (RecoilPtr32)((unsigned int)(&CZGameFrame::messageMap));
}

/**
 * Reimplements 0x4438a0: CZGameFrame::IsWindowValid.
 *
 * Purpose: treat a missing or disabled MFC window as eligible for the game
 * frame's idle dispatch checks.
 */
int __stdcall CZGameFrame::IsWindowValid(
    CWnd *pWnd
) {
    if (pWnd == 0) {
        return 0;
    }

    return pWnd->IsWindowEnabled() == 0 ? 1 : 0;
}

/**
 * Reimplements 0x4437d0: CZGameFrame::Constructor.
 *
 * Purpose: initialize the MFC frame base, game bitmap member, and game/video
 * startup hooks for the lightweight game frame.
 */
CZGameFrame * CZGameFrame::Constructor(
    const char *appId
) {
    new ((CFrameWnd *)(this)) CFrameWnd();
    new (&m_gameBitmap) CBitmap();
    RecoilApp::InitStdLogFiles(appId);
    zVideo::ModuleInit();
    return this;
}

/**
 * Reimplements 0x443830: CZGameFrame::Destructor.
 *
 * Purpose: tear down the lightweight game frame bitmap, video hook, and MFC
 * CFrameWnd provider base.
 */
void CZGameFrame::Destructor() {
    zVideo::ReturnSuccessStub();
    m_gameBitmap.DeleteObject();
    m_gameBitmap.CBitmap::~CBitmap();
    CallMfcCFrameWndDestructor((CFrameWnd *)(this));
}

/**
 * Reimplements 0x4438c0: CZGameFrame::BuildWindowTitle.
 *
 * Purpose: construct the fixed Zipper Interactive title used by the game frame.
 */
CString * CZGameFrame::BuildWindowTitle(
    CString *outTitle
) {
    new (outTitle) CString("Zipper Interactive");
    return outTitle;
}

/**
 * Reimplements 0x443a60: CZGameFrame::OnCreate.
 *
 * Purpose: finish MFC frame creation by loading the game bitmap and shutting
 * down the startup mouse device path.
 */
int CZGameFrame::OnCreate(
    CREATESTRUCTA *createStruct
) {
    const int result = CFrameWnd::OnCreate(createStruct);
    if (result == -1) {
        return result;
    }

    m_gameBitmap.Attach(
        LoadBitmapA(AfxFindResourceHandle(
            "GAMEBMP",
            MAKEINTRESOURCEA(2)
        ), "GAMEBMP")
    );
    zInput::Mouse_ShutdownDevice();
    return 0;
}

/**
 * Reimplements 0x4438f0: CZGameFrame::OnClose.
 *
 * Purpose: forward close handling to the MFC CFrameWnd provider base.
 */
void CZGameFrame::OnClose() {
    CFrameWnd::OnClose();
}

/**
 * Reimplements 0x443900: CZGameFrame::OnPaint.
 *
 * Purpose: paint the startup game bitmap into the frame unless the 3dfx client
 * rectangle update path is active.
 */
void CZGameFrame::OnPaint() {
    CPaintDC paintDc((CWnd *)(void *)this);
    if (zVid::QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        return;
    }

    PAINTSTRUCT paintStruct = paintDc.m_ps;
    HDC compatibleDc = CreateCompatibleDC(paintDc.m_hDC);
    SelectObject(
        compatibleDc,
        m_gameBitmap.m_hObject
    );

    const int paintLeft = paintStruct.rcPaint.left;
    const int paintTop = paintStruct.rcPaint.top;
    const int paintWidth = paintStruct.rcPaint.right - paintLeft;
    const int paintHeight = paintStruct.rcPaint.bottom - paintTop;
    if (paintHeight > 480) {
        StretchBlt(
            paintDc.m_hDC,
            paintLeft,
            paintTop,
            paintWidth,
            paintHeight,
            compatibleDc,
            paintLeft,
            paintTop,
            640,
            480,
            SRCCOPY
        );
    } else {
        BitBlt(
            paintDc.m_hDC,
            paintLeft,
            paintTop,
            paintWidth,
            paintHeight,
            compatibleDc,
            paintLeft,
            paintTop,
            SRCCOPY
        );
    }

    DeleteDC(compatibleDc);
}

/**
 * Reimplements 0x443ab0: CZGameFrame::OnDestroy.
 *
 * Purpose: release network/video/audio frame resources before the MFC destroy
 * handler and bitmap cleanup run.
 */
void CZGameFrame::OnDestroy() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    zVideo::ShutdownVideoSystem();
    zSndCd::Stop();
    CFrameWnd::OnDestroy();
    m_gameBitmap.DeleteObject();
}

/**
 * Reimplements 0x443ae0: CZGameFrame::OnActivate.
 *
 * Purpose: forward activation to MFC and synchronize Recoil app, input, game,
 * and video activation state.
 */
void CZGameFrame::OnActivate(
    unsigned int nState,
    CWnd *pWndOther,
    BOOL bMinimized
) {
    CFrameWnd::OnActivate(
        nState,
        pWndOther,
        bMinimized
    );

    RecoilApp_IState *const currentState = m_app->GetCurrentState();
    if (currentState != 0) {
        currentState->OnWndActivate(nState);
    }

    if (nState == 0) {
        m_app->OnAppDeactivate();
        zInput::OnAppDeactivate();
        zGame::ReturnOnlyStub();
    } else {
        m_app->OnAppActivate();
        zInput::OnAppActivate();
        zVideo_RestoreIconicFullscreenWindowIfNeeded();
    }
}

/**
 * Reimplements 0x443a20: CZGameFrame::OnSize.
 *
 * Purpose: let MFC handle resizing and refresh the cached video client rect
 * when the update mask requests it.
 */
void CZGameFrame::OnSize(
    unsigned int nType,
    int cx,
    int cy
) {
    CFrameWnd::OnSize(
        nType,
        cx,
        cy
    );
    zVid::UpdateCachedClientRectIfUpdateMaskEnabled();
}

/**
 * Reimplements 0x443a50: CZGameFrame::OnMove.
 *
 * Purpose: dispatch default MFC move handling and refresh the cached video
 * client rect when the update mask requests it.
 */
void CZGameFrame::OnMove(
    int,
    int
) {
    Default();
    zVid::UpdateCachedClientRectIfUpdateMaskEnabled();
}

/**
 * Reimplements 0x443b50: CZGameFrame::OnAppIdleDispatchMessage.
 *
 * Purpose: route frame idle/dispatch work into the current Recoil application
 * object.
 */
int CZGameFrame::OnAppIdleDispatchMessage(
    unsigned int wParam,
    unsigned int lParam
) {
    return m_app->OnIdleOrDispatch(
        wParam,
        lParam
    );
}
