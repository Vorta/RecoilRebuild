#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
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

    CZGameFrame();
    CZGameFrame(const char *appId);
    ~CZGameFrame();
    static CRuntimeClass *__stdcall GetBaseRuntimeClass();
    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    static CZGameFrame *CreateObject();
    static CRuntimeClass *__stdcall GetRuntimeClassStatic();
    virtual CRuntimeClass *GetRuntimeClass() const;
    static const AFX_MSGMAP *__stdcall GetBaseMessageMap();
    static const AFX_MSGMAP *__stdcall GetMessageMapStatic();
    virtual const AFX_MSGMAP * GetMessageMap() const;
    virtual int IsWindowValid(CWnd *pWnd) const;
    virtual CString * BuildWindowTitle(CString *outTitle);
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
