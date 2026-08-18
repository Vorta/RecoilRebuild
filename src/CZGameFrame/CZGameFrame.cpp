#include "CZGameFrame.h"

#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zVideo/zvid.h"

HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

namespace zSndCd {
int Stop();
}

extern "C" {
/**
 *
 * Purpose: provide the recovered app id string used by the CZGameFrame
 * runtime-class factory.
 */
extern const char g_CZGameFrame_DefaultAppId[] = "gamez";

/**
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

/*
 * BN evidence for 0x4438a0 loads the explicit CWnd argument's object pointer
 * and calls the fifth virtual entry. MFC42 CWnd::IsWindowEnabled is
 * nonvirtual/provider-backed and would add a retail-absent import, so keep
 * this as a narrow query view without assigning a provider method name.
 */
struct MfcWindowValidityTarget {
    virtual int ProviderEntry0() const;
    virtual int ProviderEntry1() const;
    virtual int ProviderEntry2() const;
    virtual int ProviderEntry3() const;
    virtual int QueryWindowValidity() const;
};

} // namespace

/**
 *
 * Purpose: use the original VC5SP3 MFC dynamic-creation product to expose the
 * frame factory, provider base-runtime callback, runtime-class record, and
 * virtual runtime-class accessor in their natural emitted order.
 */
IMPLEMENT_DYNCREATE(CZGameFrame, CFrameWnd)

/**
 *
 * Purpose: use the original VC5SP3 MFC message-map product for the frame's
 * base-map callback, virtual map accessor, map record, and terminal entries.
 */
BEGIN_MESSAGE_MAP(CZGameFrame, CFrameWnd)
    ON_WM_CLOSE()
    ON_WM_PAINT()
    ON_WM_SIZE()
    ON_WM_MOVE()
    ON_WM_CREATE()
    ON_WM_DESTROY()
    ON_MESSAGE(0x3b9, OnAppIdleDispatchMessage)
    ON_WM_ACTIVATE()
END_MESSAGE_MAP()

/**
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
 *
 * Purpose: let compiler-emitted MFC-derived teardown restore provider vtables
 * and release the owned game bitmap member before the CFrameWnd base.
 */
CZGameFrame::~CZGameFrame() {
    zVideo::ReturnSuccessStub();
    m_gameBitmap.DeleteObject();
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-iswindowvalid
 * @recoil-artifact defines .text recoil:function:0x4438a0: CZGameFrame::IsWindowValid.
 *
 * Purpose: preserve the frame vtable callback shape for the MFC
 * window-validity rule.
 */
int CZGameFrame::IsWindowValid(
    CWnd *pWnd
) const {
    if (pWnd != 0) {
        return ((const MfcWindowValidityTarget *)(const void *)pWnd)
            ->QueryWindowValidity() == 0 ? 1 : 0;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-buildwindowtitle
 * @recoil-artifact defines .text recoil:function:0x4438c0: CZGameFrame::BuildWindowTitle.
 *
 * Purpose: construct the fixed Zipper Interactive title used by the game frame.
 */
CString * CZGameFrame::BuildWindowTitle(
    CString *outTitle
) {
    volatile int constructedTitleState = 0;
    outTitle->CString::CString("Zipper Interactive");
    return outTitle;
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onclose
 * @recoil-artifact defines .text recoil:function:0x4438f0: CZGameFrame::OnClose.
 *
 * Purpose: forward close handling to the MFC CFrameWnd provider base.
 */
void CZGameFrame::OnClose() {
    CFrameWnd::OnClose();
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onpaint
 * @recoil-artifact defines .text recoil:function:0x443900: CZGameFrame::OnPaint.
 *
 * Purpose: paint the startup game bitmap into the frame unless the 3dfx client
 * rectangle update path is active.
 */
void CZGameFrame::OnPaint() {
    CPaintDC paintDc((CWnd *)(void *)this);
    if (zVid::QueryCachedClientRectUpdateMaskIf3dfx() == 0) {
        PAINTSTRUCT paintStruct = paintDc.m_ps;
        HDC compatibleDc = CreateCompatibleDC(paintDc.GetSafeHdc());
        SelectObject(
            compatibleDc,
            m_gameBitmap.GetSafeHandle()
        );

        if (paintStruct.rcPaint.bottom - paintStruct.rcPaint.top > 480) {
            StretchBlt(
                paintDc.GetSafeHdc(),
                paintStruct.rcPaint.left,
                paintStruct.rcPaint.top,
                paintStruct.rcPaint.right - paintStruct.rcPaint.left,
                paintStruct.rcPaint.bottom - paintStruct.rcPaint.top,
                compatibleDc,
                paintStruct.rcPaint.left,
                paintStruct.rcPaint.top,
                640,
                480,
                SRCCOPY
            );
        } else {
            BitBlt(
                paintDc.GetSafeHdc(),
                paintStruct.rcPaint.left,
                paintStruct.rcPaint.top,
                paintStruct.rcPaint.right - paintStruct.rcPaint.left,
                paintStruct.rcPaint.bottom - paintStruct.rcPaint.top,
                compatibleDc,
                paintStruct.rcPaint.left,
                paintStruct.rcPaint.top,
                SRCCOPY
            );
        }

        DeleteDC(compatibleDc);
    }
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onsize
 * @recoil-artifact defines .text recoil:function:0x443a20: CZGameFrame::OnSize.
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
 * Purpose: refresh the cached client rectangle when the active renderer path
 * permits update-mask-driven window tracking.
 */
void __cdecl zVid::UpdateCachedClientRectIfUpdateMaskEnabled() {
    if (QueryCachedClientRectUpdateMaskIf3dfx() != 0) {
        zVideo::UpdateCachedClientRectScreenCoords();
    }
}

/**
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onmove
 * @recoil-artifact defines .text recoil:function:0x443a50: CZGameFrame::OnMove.
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
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-oncreate
 * @recoil-artifact defines .text recoil:function:0x443a60: CZGameFrame::OnCreate.
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
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-ondestroy
 * @recoil-artifact defines .text recoil:function:0x443ab0: CZGameFrame::OnDestroy.
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
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onactivate
 * @recoil-artifact defines .text recoil:function:0x443ae0: CZGameFrame::OnActivate.
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
 * @recoil-anchor recoil:anchor:src-czgameframe-czgameframe-function-czgameframe-onappidledispatchmessage
 * @recoil-artifact defines .text recoil:function:0x443b50: CZGameFrame::OnAppIdleDispatchMessage.
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
