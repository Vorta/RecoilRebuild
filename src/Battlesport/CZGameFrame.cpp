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

extern "C" {
/**
 * Reimplements data 0x4dd8e8: g_CZGameFrame_DefaultAppId.
 *
 * Purpose: provide the recovered app id string used by the CZGameFrame
 * runtime-class factory.
 */
extern const char g_CZGameFrame_DefaultAppId[] = "gamez";

/**
 * Reimplements data 0x4dd904: g_CZGameFrame_GameBmpResourceName.
 *
 * Purpose: name the recovered bitmap resource loaded by CZGameFrame creation.
 */
extern const char g_CZGameFrame_GameBmpResourceName[] = "GAMEBMP";
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
typedef CObject *(PASCAL *MfcCreateObjectProc)();

const UINT kMfcMessageMapSigIntCreateStruct = 9;
const UINT kMfcMessageMapSigLongWparamLparam = 10;
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidIntInt = 15;
const UINT kMfcMessageMapSigVoidUIntIntInt = 17;
const UINT kMfcMessageMapSigVoidUIntCWndBool = 28;

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
    &CZGameFrame::GetBaseMessageMap,
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
    &CZGameFrame::GetBaseRuntimeClass,
    0,
};

/**
 * Reimplements 0x443790: CZGameFrame::GetBaseRuntimeClass.
 *
 * Purpose: return the MFC CFrameWnd runtime-class symbol for CZGameFrame's
 * recovered runtime-class hierarchy.
 */
CRuntimeClass *__stdcall CZGameFrame::GetBaseRuntimeClass() {
    return (CRuntimeClass *)&CFrameWnd::classCFrameWnd;
}

/**
 * Reimplements 0x443730: CZGameFrame::CreateObject.
 *
 * Purpose: allocate and construct the game frame object for the recovered MFC
 * runtime-class factory path.
 */
CZGameFrame *CZGameFrame::CreateObject() {
    return new CZGameFrame(g_CZGameFrame_DefaultAppId);
}

/**
 * Original helper evidence: no standalone retail default-constructor address;
 * source/local construction paths use it only to install the compiler-emitted
 * MFC-derived vtable.
 *
 * Purpose: let normal C++ construction install the MFC-derived vtable and
 * provider-owned base/member state without running the app-shell startup hooks.
 */
CZGameFrame::CZGameFrame() {
}

/**
 * Reimplements 0x4437d0: CZGameFrame::CZGameFrame.
 *
 * Purpose: model the original MFC-derived construction path that installs the
 * compiler-emitted CZGameFrame vtable before the app-shell startup hooks run.
 */
CZGameFrame::CZGameFrame(
    const char *appId
) {
    RecoilApp::InitStdLogFiles(appId);
    zVideo::ModuleInit();
}

/**
 * Reimplements 0x4437a0 callback rule: CZGameFrame runtime-class access.
 *
 * Purpose: keep a static callback for MFC data records while the vtable slot is
 * modeled by the non-static MFC override.
 */
CRuntimeClass *__stdcall CZGameFrame::GetRuntimeClassStatic() {
    return &CZGameFrame::classCZGameFrame;
}

/**
 * Reimplements 0x4437a0: CZGameFrame::GetRuntimeClass.
 *
 * Purpose: expose CZGameFrame's runtime-class record through the inherited MFC
 * virtual slot that owns the first entry of the compiler-emitted frame vtable.
 */
CRuntimeClass *CZGameFrame::GetRuntimeClass() const {
    return &CZGameFrame::classCZGameFrame;
}

/**
 * Reimplements 0x4437b0: CZGameFrame::GetBaseMessageMap.
 *
 * Purpose: return the provider CFrameWnd message-map symbol for the frame's
 * recovered message-map hierarchy.
 */
const AFX_MSGMAP *__stdcall CZGameFrame::GetBaseMessageMap() {
    return &CFrameWnd::messageMap;
}

/**
 * Reimplements 0x4437c0 callback rule: CZGameFrame message-map access.
 *
 * Purpose: keep a static callback for MFC data records while the vtable slot is
 * modeled by the non-static MFC override.
 */
const AFX_MSGMAP *__stdcall CZGameFrame::GetMessageMapStatic() {
    return &CZGameFrame::messageMap;
}

/**
 * Reimplements 0x4437c0: CZGameFrame::GetMessageMap.
 *
 * Purpose: expose CZGameFrame's message-map record through the inherited MFC
 * virtual slot used by MFC command and window-message dispatch.
 */
const AFX_MSGMAP * CZGameFrame::GetMessageMap() const {
    return &CZGameFrame::messageMap;
}

/**
 * Reimplements 0x4438a0: CZGameFrame::IsWindowValid.
 *
 * Purpose: preserve the frame vtable callback shape for the MFC
 * window-validity rule.
 */
int CZGameFrame::IsWindowValid(
    CWnd *pWnd
) const {
    if (pWnd == 0) {
        return 0;
    }

    return pWnd->IsWindowEnabled() == 0 ? 1 : 0;
}

/**
 * Reimplements 0x443830: CZGameFrame::~CZGameFrame.
 *
 * Purpose: let compiler-emitted MFC-derived teardown restore provider vtables
 * and release the owned game bitmap member before the CFrameWnd base.
 */
CZGameFrame::~CZGameFrame() {
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
            g_CZGameFrame_GameBmpResourceName,
            MAKEINTRESOURCEA(2)
        ), g_CZGameFrame_GameBmpResourceName)
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
