#include "Battlesport/GameNet.h"

#include <cstring>

extern "C" int net_format_ipv4_address_smoke(void)
{
    char text[32];

    Net::FormatIpv4Address(text, 0);
    if (std::strcmp(text, "0.0.0.0") != 0) {
        return 1;
    }

    Net::FormatIpv4Address(text, 0x01020304);
    if (std::strcmp(text, "4.3.2.1") != 0) {
        return 2;
    }

    Net::FormatIpv4Address(text, 0xffffffff);
    if (std::strcmp(text, "255.255.255.255") != 0) {
        return 3;
    }

    Net::FormatIpv4Address(text, 0xc0a80164);
    if (std::strcmp(text, "100.1.168.192") != 0) {
        return 4;
    }

    return 0;
}
