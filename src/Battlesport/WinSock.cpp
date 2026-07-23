#include "Battlesport/game_net.h"
#include "Battlesport/net_ui.h"

#include <winsock2.h>
#include <windows.h>

#include <stdio.h>

namespace NetUi {

/**
 * Reimplements 0x43ce80: NetUi::VerifyWinsock2OrPromptContinue
 * (D:\Proj\Battlesport\Net\NetUi.cpp).
 *
 * Purpose: verify Winsock 2.0 availability and ask the user whether network
 * flows should continue when startup or version checks fail.
 */
int __fastcall VerifyWinsock2OrPromptContinue(
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

namespace Net {

/**
 * Reimplements 0x43cf40: Net::FormatIpv4Address
 * Physical host: provisional no-literal WinSock.cpp contribution; original
 * source provenance remains unresolved.
 * Purpose: Format a little-endian IPv4 address for session UI text.
 */
void __fastcall FormatIpv4Address(
    char *outText,
    unsigned int ipAddress
) {
    int octets[4];
    int index;
    for (index = 0; index < 4; ++index) {
        octets[index] = (int)(ipAddress & 0xff);
        ipAddress >>= 8;
    }

    sprintf(
        outText,
        /* Retail literal 0x4dd250 is the anonymous compiler string for the
           dotted-quad IPv4 format; data ownership remains blocked outside
           this source slice. */
        "%d.%d.%d.%d",
        octets[0],
        octets[1],
        octets[2],
        octets[3]
    );
}

} // namespace Net
