#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/RecoilApp.h"
#include "recoil/recoil_callconv.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_GAME_FRAME_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_GAME_FRAME_NOINLINE __attribute__((noinline))
#else
#define RECOIL_GAME_FRAME_NOINLINE
#endif

struct CZGameFrameMfcWindow {
    void **vftable;
};

struct CZGameFrameApp;

struct CZGameFrameAppVtable {
    unsigned char reserved000[0xa4];
    void(RECOIL_THISCALL *OnAppActivate)(CZGameFrameApp *self);
    void(RECOIL_THISCALL *OnAppDeactivate)(CZGameFrameApp *self);
    unsigned char reserved0ac[0x08];
    int(RECOIL_THISCALL *OnIdleOrDispatch)(CZGameFrameApp *self, unsigned int wParam,
                                                    unsigned int lParam);
};

struct CZGameFrameApp {
    CZGameFrameAppVtable *vftable;
};

class CWnd;

class CGdiObject {
  public:
    void *vftable;
    HGDIOBJ m_hObject;

    BOOL Attach(HGDIOBJ hObject);
    BOOL DeleteObject();
};

class CFrameWnd {
  public:
    CFrameWnd();

    __declspec(dllimport) static const CRuntimeClass classCFrameWnd;

  protected:
    __declspec(dllimport) static const AFX_MSGMAP messageMap;

    int OnCreate(CREATESTRUCTA *createStruct);
    void OnClose();
    void OnDestroy();
    void OnActivate(unsigned int nState, CWnd *pWndOther, BOOL bMinimized);
    void OnSize(unsigned int nType, int cx, int cy);
};

struct CZGameFrame : CFrameWnd {
    unsigned char reserved000[0xc0];
    CZGameFrameApp *m_app;
    CGdiObject m_gameBitmap;

    static CRuntimeClass classCZGameFrame;
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetBaseRuntimeClass();
    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_GAME_FRAME_NOINLINE static CZGameFrame *RECOIL_CDECL CreateObject();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetRuntimeClass();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetBaseMessageMap();
    RECOIL_GAME_FRAME_NOINLINE static RecoilPtr32 RECOIL_CDECL GetMessageMap();
    RECOIL_GAME_FRAME_NOINLINE static int RECOIL_STDCALL
    IsWindowValid(CZGameFrameMfcWindow *pWnd);
    RECOIL_GAME_FRAME_NOINLINE CZGameFrame *RECOIL_THISCALL Constructor(const char *appId);
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_GAME_FRAME_NOINLINE CString *RECOIL_THISCALL BuildWindowTitle(CString *outTitle);
    RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL OnCreate(CREATESTRUCTA *createStruct);
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnClose();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnPaint();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnDestroy();
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnActivate(unsigned int nState,
                                                               CWnd *pWndOther, BOOL bMinimized);
    RECOIL_GAME_FRAME_NOINLINE void RECOIL_THISCALL OnSize(unsigned int nType, int cx,
                                                           int cy);
    RECOIL_GAME_FRAME_NOINLINE int RECOIL_THISCALL
    OnAppIdleDispatchMessage(unsigned int wParam, unsigned int lParam);
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(offsetof(CGdiObject, m_hObject) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(CZGameFrameAppVtable, OnAppActivate) == 0x0a4);
RECOIL_STATIC_ASSERT(offsetof(CZGameFrameAppVtable, OnAppDeactivate) == 0x0a8);
RECOIL_STATIC_ASSERT(offsetof(CZGameFrameAppVtable, OnIdleOrDispatch) == 0x0b4);
RECOIL_STATIC_ASSERT(offsetof(CZGameFrame, m_app) == 0x0c0);
RECOIL_STATIC_ASSERT(offsetof(CZGameFrame, m_gameBitmap) == 0x0c4);
#endif
