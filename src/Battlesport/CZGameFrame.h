#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

// Authored Recoil game frame reconstructed over imported MFC42 CFrameWnd and
// GDI providers; MFC base behavior is not reimplemented here.
struct CZGameFrame : CFrameWnd {
    RecoilApp *m_app;
    CBitmap m_gameBitmap;

    static CRuntimeClass classCZGameFrame;
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static RecoilPtr32 GetBaseRuntimeClass();
    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    static CZGameFrame *CreateObject();
    static RecoilPtr32 GetRuntimeClass();
    static RecoilPtr32 GetBaseMessageMap();
    static RecoilPtr32 GetMessageMap();
    static int __stdcall IsWindowValid(CWnd *pWnd);
    CZGameFrame * Constructor(const char *appId);
    void Destructor();
    CString * BuildWindowTitle(CString *outTitle);
    int OnCreate(CREATESTRUCTA *createStruct);
    void OnClose();
    void OnPaint();
    void OnDestroy();
    void OnActivate(
        unsigned int nState,
        CWnd *pWndOther,
        BOOL bMinimized
    );
    void OnSize(
        unsigned int nType,
        int cx,
        int cy
    );
    void OnMove(
        int x,
        int y
    );
    int OnAppIdleDispatchMessage(
        unsigned int wParam,
        unsigned int lParam
    );
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(
    offsetof(
        CGdiObject,
        m_hObject
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZGameFrame,
        m_app
    ) == 0x0c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZGameFrame,
        m_gameBitmap
    ) == 0x0c4
);
#endif
