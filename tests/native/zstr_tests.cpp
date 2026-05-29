#include "Battlesport/zStr.h"

extern "C" int zstr_contains_case_insensitive_smoke(void)
{
    char longHaystack[150];
    for (int index = 0; index < 149; ++index) {
        longHaystack[index] = 'a';
    }
    longHaystack[129] = 'Z';
    longHaystack[149] = '\0';

    char boundaryHaystack[150];
    for (int index = 0; index < 149; ++index) {
        boundaryHaystack[index] = 'b';
    }
    boundaryHaystack[127] = 'Q';
    boundaryHaystack[149] = '\0';

    const bool basicMatch =
        zStr::ContainsCaseInsensitive("ReCoIl Cheat Command", "cheat") == 1;
    const bool missing =
        zStr::ContainsCaseInsensitive("ReCoIl Cheat Command", "briefing") == 0;
    const bool emptyNeedle = zStr::ContainsCaseInsensitive("anything", "") == 1;
    const bool emptyHaystack = zStr::ContainsCaseInsensitive("", "anything") == 0;
    const bool bothEmpty = zStr::ContainsCaseInsensitive("", "") == 1;
    const bool truncatedMiss = zStr::ContainsCaseInsensitive(longHaystack, "z") == 0;
    const bool boundaryMatch = zStr::ContainsCaseInsensitive(boundaryHaystack, "q") == 1;

    return basicMatch && missing && emptyNeedle && emptyHaystack && bothEmpty &&
                   truncatedMiss && boundaryMatch
               ? 0
               : 1;
}
