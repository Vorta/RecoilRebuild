#include "Battlesport/zstr.h"

#include <ctype.h>
#include <string.h>

namespace zStr {

const int kContainsCaseInsensitiveBufferChars = 128;

/**
 * Reimplements 0x406a00: zStr::ContainsCaseInsensitive
 * (D:\Proj\Battlesport\zStr.cpp).
 *
 * Purpose: compare uppercase bounded copies of two strings and report whether
 * the needle appears in the haystack.
 */
int __fastcall ContainsCaseInsensitive(
    const char *haystack,
    const char *needle
) {
    char uppercaseHaystack[kContainsCaseInsensitiveBufferChars + 1];
    char uppercaseNeedle[kContainsCaseInsensitiveBufferChars + 1];

    int haystackIndex = 0;
    while (
        haystack[haystackIndex] != '\0' &&
        haystackIndex < kContainsCaseInsensitiveBufferChars
    ) {
        uppercaseHaystack[haystackIndex] = (char)toupper(haystack[haystackIndex]);
        ++haystackIndex;
    }
    uppercaseHaystack[haystackIndex] = '\0';

    int needleIndex = 0;
    while (
        needle[needleIndex] != '\0' &&
        needleIndex < kContainsCaseInsensitiveBufferChars
    ) {
        uppercaseNeedle[needleIndex] = (char)toupper(needle[needleIndex]);
        ++needleIndex;
    }
    uppercaseNeedle[needleIndex] = '\0';

    return strstr(
        uppercaseHaystack,
        uppercaseNeedle
    ) != 0 ? 1 : 0;
}

} // namespace zStr
