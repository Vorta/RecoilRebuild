#include "Battlesport/NetUi.h"

#include <windows.h>
#include <winsock2.h>

#include <stdio.h>

namespace NetUi {

// Reimplements 0x43ce80: NetUi::VerifyWinsock2OrPromptContinue
// (D:\Proj\Battlesport\Net\NetUi.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL VerifyWinsock2OrPromptContinue(
    const char *caption,
    const char *messageFormat
) {
    int result = TRUE;
    WSADATA wsaData;
    if (WSAStartup(
        2,
        &wsaData
    ) != 0) {
        result = FALSE;
    } else if (LOBYTE(wsaData.wHighVersion) != 2 || HIBYTE(wsaData.wHighVersion) != 0) {
        WSACleanup();
        result = FALSE;
    }

    if (result == 0) {
        char promptText[512];
        sprintf(
            promptText,
            messageFormat,
            (unsigned int)LOBYTE(wsaData.wHighVersion),
            (unsigned int)HIBYTE(wsaData.wHighVersion)
        );
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(
            GetFocus(),
            promptText,
            caption,
            MB_ICONQUESTION | MB_YESNO
        ) == IDYES) {
            result = TRUE;
        }
    }

    return result;
}

} // namespace NetUi
