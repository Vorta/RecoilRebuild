#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/Mfc42Abi.h"
#include "Battlesport/recoil_app.h"
#include "recoil/recoil_callconv.h"

extern "C" {
extern const char g_CZGameFrame_DefaultAppId[];
}

/**
 * Authored Recoil game frame reconstructed over imported MFC42 CFrameWnd and
 * GDI providers; MFC base behavior is not reimplemented here.
 * Emits 0x443810: VC5 compiler-generated scalar deleting destructor for the
 * virtual CZGameFrame destructor model; not a separate authored body.
 * Emits 0x443b70: provider-owned CGdiObject scalar deleting destructor induced
 * by the complete MFC/GDI class and member closure.
 * Emits 0x443be0: provider-owned CBitmap scalar deleting destructor induced by
 * the complete MFC/GDI class and member closure.
 */
struct CZGameFrame : CFrameWnd {
    RecoilApp *m_app;
    CBitmap m_gameBitmap;

    DECLARE_DYNCREATE(CZGameFrame)

    CZGameFrame(const char *appId = g_CZGameFrame_DefaultAppId);
    ~CZGameFrame();
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

private:
    static const AFX_MSGMAP_ENTRY _messageEntries[];

protected:
    static AFX_DATA const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP *PASCAL _GetBaseMessageMap();

public:
    virtual const AFX_MSGMAP *GetMessageMap() const;
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
