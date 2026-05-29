#pragma once

#include "recoil/recoil_types.h"
#include <map>
#include <string>

namespace zbd {
struct Header {
    unsigned int magic_00;   // 0x02971222
    unsigned int version_04; // 0x0F

    unsigned int texDirArg_08;    // passed to zImage_ReadTextureDirectory
    unsigned int texDirOffset_0C; // file offset

    unsigned int matlOffset_10;    // file offset
    unsigned int model3dOffset_14; // file offset

    unsigned int nodeCount_18;
    int nodeFreeHead_1C;
    unsigned int nodeTableOffset_20;
};

struct Summary {
    Header header;
    unsigned __int64 fileSize;
    bool headerValid;
    map<unsigned int, unsigned int> classTypeHistogram; // classType_34 -> count
};

// Returns a human-readable summary. If parsing fails, returns an error message.
wstring ReadZbdSummaryText(const wstring &path);

// More detailed report for reverse engineering / validation.
// Includes node table stats and per-node payload offsets (flags_C0 & 0x00FFFFFF).
wstring ReadZbdDetailedText(const wstring &path);

// CSV export of node table records (one row per node).
// NOTE: Returned text is UTF-16; the launcher currently writes UTF-16LE with BOM.
// Columns:
//   index,nameHex,classType,payloadOff24,flags,nodeRecOff,classDataPtr_38,actionCallback_48,
//   listCountA_54,listA_ptr_58,listCountB_5C,listB_ptr_60
wstring ReadZbdNodesCsvText(const wstring &path);
} // namespace zbd
