#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_GAME_FRAME_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_GAME_FRAME_NOINLINE __attribute__((noinline))
#else
#define RECOIL_GAME_FRAME_NOINLINE
#endif

// Authored Recoil game frame reconstructed over imported MFC42 CFrameWnd and
// GDI providers; MFC base behavior is not reimplemented here.
struct CZGameFrame : CFrameWnd {
    RecoilApp *m_app;
    CBitmap m_gameBitmap;

    static CRuntimeClass classCZGameFrame;
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetBaseRuntimeClass();
    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_GAME_FRAME_NOINLINE static CZGameFrame *RECOIL_CDECL CreateObject();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetRuntimeClass();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetBaseMessageMap();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetMessageMap();
    RECOIL_GAME_FRAME_NOINLINE static int RECOIL_STDCALL IsWindowValid(CWnd *pWnd);
    RECOIL_GAME_FRAME_NOINLINE CZGameFrame *RECOIL_THISCALL Constructor(const char *appId);
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_GAME_FRAME_NOINLINE CString *RECOIL_THISCALL BuildWindowTitle(CString *outTitle);
    RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL OnCreate(CREATESTRUCTA *createStruct);
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnClose();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnPaint();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnDestroy();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnActivate(
        unsigned int nState,
        CWnd *pWndOther,
        BOOL bMinimized
    );
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnSize(
        unsigned int nType,
        int cx,
        int cy
    );
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnMove(
        int x,
        int y
    );
    RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL OnAppIdleDispatchMessage(
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
