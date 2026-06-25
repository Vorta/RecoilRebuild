extern "C" {
#include "../../src/Messages/messages.c"
}

extern "C" int zloc_get_id_messages_lookup_smoke(void)
{
    if (ZLocGetID("MSG_BACK") != 1u) {
        return 1;
    }
    if (ZLocGetID("MSG_WOL_RESTART_REQUIRED") != 0x3046u) {
        return 2;
    }
    if (ZLocGetID("MSG_DOES_NOT_EXIST") != 0u) {
        return 3;
    }
    if (ZLocGetID("msg_back") != 0u) {
        return 4;
    }
    if (ZLocGetID("MSG_BACK ") != 0u) {
        return 5;
    }

    return 0;
}
