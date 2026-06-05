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
const RecoilNamedVtable kCZGameFrame_Vtable = {"CZGameFrame vtable"};
typedef void( *RecoilStateWndActivateMethod)(
    RecoilApp_IState *,
    unsigned int
);
typedef void( *CFrameWndDestructorProc)(CFrameWnd *);

/**
 * Local MFC provider boundary helper for CZGameFrame runtime-class/message-map
 * thunks.
 *
 * Purpose: return source symbols through the 32-bit pointer value shape used by
 * the recovered retail accessors.
 */
RecoilPtr32 Ptr32FromSymbol(
    const void *symbol
) {
    return (RecoilPtr32)((unsigned int)(symbol));
}

/**
 * Local MFC provider boundary callback for CZGameFrame::classCZGameFrame.
 *
 * Purpose: expose the CFrameWnd provider runtime class through the callback
 * shape expected by the recovered MFC class record.
 */
CRuntimeClass *__stdcall GetCZGameFrameBaseRuntimeClass() {
    return (CRuntimeClass *)(&CFrameWnd::classCFrameWnd);
}

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

AFX_MSGMAP_ENTRY const CZGameFrame::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP CZGameFrame::messageMap = {
    &CZGameFrame::GetBaseMessageMapForMfc,
    &CZGameFrame::messageEntries[0],
};

CRuntimeClass CZGameFrame::classCZGameFrame = {
    "CZGameFrame",
    sizeof(CZGameFrame),
    0xffff,
    0,
    &GetCZGameFrameBaseRuntimeClass,
    0,
};

/**
 * Reimplements 0x443790: CZGameFrame::GetBaseRuntimeClass.
 *
 * Purpose: return the MFC CFrameWnd runtime-class symbol for CZGameFrame's
 * recovered runtime-class hierarchy.
 */
RecoilPtr32 CZGameFrame::GetBaseRuntimeClass() {
    return Ptr32FromSymbol(&CFrameWnd::classCFrameWnd);
}

/**
 * Local MFC provider boundary message-map callback for CZGameFrame::messageMap.
 *
 * Purpose: expose CFrameWnd's provider message map through the callback shape
 * stored in the recovered CZGameFrame message-map record.
 */
const AFX_MSGMAP *__stdcall CZGameFrame::GetBaseMessageMapForMfc() {
    return &CFrameWnd::messageMap;
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
    return Ptr32FromSymbol(&CZGameFrame::classCZGameFrame);
}

/**
 * Reimplements 0x4437b0: CZGameFrame::GetBaseMessageMap.
 *
 * Purpose: return the provider CFrameWnd message-map symbol for the frame's
 * recovered message-map hierarchy.
 */
RecoilPtr32 CZGameFrame::GetBaseMessageMap() {
    return Ptr32FromSymbol(&CFrameWnd::messageMap);
}

/**
 * Reimplements 0x4437c0: CZGameFrame::GetMessageMap.
 *
 * Purpose: return CZGameFrame's recovered MFC message-map record.
 */
RecoilPtr32 CZGameFrame::GetMessageMap() {
    return Ptr32FromSymbol(&CZGameFrame::messageMap);
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
 * Purpose: initialize the MFC frame base, game bitmap member, frame vtable, and
 * game/video startup hooks for the lightweight game frame.
 */
CZGameFrame * CZGameFrame::Constructor(
    const char *appId
) {
    new ((CFrameWnd *)(this)) CFrameWnd();
    new (&m_gameBitmap) CBitmap();
    *(RecoilPtr32 *)(this) = Ptr32FromSymbol(&kCZGameFrame_Vtable);
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
    *(RecoilPtr32 *)(this) = Ptr32FromSymbol(&kCZGameFrame_Vtable);
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
    if (zVid_QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
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
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
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
    zVid_UpdateCachedClientRectIfUpdateMaskEnabled();
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
