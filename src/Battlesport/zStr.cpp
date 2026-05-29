#include "Battlesport/zStr.h"

#include <ctype.h>
#include <string.h>

namespace zStr {

const int kContainsCaseInsensitiveBufferChars = 128;

static void CopyUppercasePrefix(char *dest, const char *source)
{
    int index = 0;
    while (source[index] != '\0' && index < kContainsCaseInsensitiveBufferChars) {
        dest[index] = (char)toupper(source[index]);
        ++index;
    }
    dest[index] = '\0';
}

// Reimplements 0x406a00: zStr::ContainsCaseInsensitive (D:\Proj\Battlesport\zStr.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ContainsCaseInsensitive(const char *haystack,
                                                           const char *needle)
{
    char uppercaseHaystack[kContainsCaseInsensitiveBufferChars + 1];
    char uppercaseNeedle[kContainsCaseInsensitiveBufferChars + 1];

    CopyUppercasePrefix(uppercaseHaystack, haystack);
    CopyUppercasePrefix(uppercaseNeedle, needle);

    return strstr(uppercaseHaystack, uppercaseNeedle) != 0 ? 1 : 0;
}

}
