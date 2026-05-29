#include "zbd_reader.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#include <algorithm>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>

namespace zbd {
static const unsigned int kMagic = 0x02971222u;
static const unsigned int kVersion = 0x0Fu;

// Validation caps (perf guards). These are intended for report sanity checking, not strict parsing.
static const unsigned __int64 kNodeRefListMaxBytes = (1ui64 << 20); // 1 MiB
static const unsigned __int64 kWorldMaxCellsWalk = 500000ui64;
static const unsigned __int64 kWorldMaxTotalChildRefsToCheck = 200000ui64;
static const unsigned int kWorldChildRefChunk = 4096u;

struct NodeRecord32 {
    // Original likely: char name[0x34]. Treat as raw bytes; we only read it, not interpret.
    unsigned char name_00[0x34];

    unsigned int classType_34;
    unsigned int
        classDataPtr_38;     // 32-bit pointer in original process; file stores node records raw
    int diIndex_3C; // serialized DI pool index; fixed up in engine after load

    unsigned char pad_40[0x08];
    unsigned int actionCallback_48; // runtime-only function pointer in original engine; writer
                                     // clears it to 0 on disk
    unsigned char pad_4C[0x08];

    int listCountA_54;
    unsigned int listA_58;
    int listCountB_5C;
    unsigned int listB_60;

    unsigned char pad_64[0x5C];
    unsigned int flags_C0;
};
RECOIL_STATIC_ASSERT(sizeof(NodeRecord32) == 0xC4);

static wstring Win32LastErrorToString(DWORD err) {
    if (err == 0)
        return L"";

    LPWSTR buf = 0;
    DWORD len = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        0, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buf, 0, 0);

    wstring out;
    if (len && buf) {
        out.assign(buf, buf + len);
        // trim trailing CR/LF
        while (!out.empty() &&
               (out[out.size() - 1] == L'\r' || out[out.size() - 1] == L'\n'))
            out.pop_back();
    }
    if (buf)
        LocalFree(buf);
    return out;
}

static bool ReadExact(FILE *f, void *dst, size_t sz) {
    return fread(dst, 1, sz, f) == sz;
}

static void InitSummary(Summary &s) {
    memset(&s.header, 0, sizeof(s.header));
    s.fileSize = 0;
    s.headerValid = false;
    s.classTypeHistogram.clear();
}

static unsigned __int64 GetFileSizeOrZero(const wstring &path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &data))
        return 0;

    ULARGE_INTEGER size;
    size.HighPart = data.nFileSizeHigh;
    size.LowPart = data.nFileSizeLow;
    return size.QuadPart;
}

static bool ClassTypeCountGreater(const pair<unsigned int, unsigned int> &lhs,
                                  const pair<unsigned int, unsigned int> &rhs) {
    return lhs.second > rhs.second;
}

static unsigned int ReadU32FromBuffer(const unsigned char *buf, size_t offset) {
    unsigned int value = 0;
    memcpy(&value, buf + offset, sizeof(value));
    return value;
}

static int ReadI32FromBuffer(const unsigned char *buf, size_t offset) {
    int value = 0;
    memcpy(&value, buf + offset, sizeof(value));
    return value;
}

static wstring BytesToHex(const unsigned char *p, size_t n) {
    static const wchar_t *kHex = L"0123456789abcdef";
    wstring out;
    out.reserve(n * 2);
    for (size_t i = 0; i < n; i++) {
        const unsigned char b = p[i];
        out.push_back(kHex[(b >> 4) & 0xF]);
        out.push_back(kHex[b & 0xF]);
    }
    return out;
}

static wstring FormatSummary(const Summary &s) {
    wstringstream ss;
    ss << L"ZBD summary\n";
    ss << L"----------\n";
    ss << L"File size: " << s.fileSize << L" bytes\n";
    ss << L"Header valid: " << (s.headerValid ? L"yes" : L"no") << L"\n";
    ss << L"Magic: 0x" << hex << uppercase << s.header.magic_00 << dec << L"\n";
    ss << L"Version: " << s.header.version_04 << L"\n";
    ss << L"TexDir arg: " << s.header.texDirArg_08 << L"\n";
    ss << L"TexDir offset: 0x" << hex << s.header.texDirOffset_0C << dec << L"\n";
    ss << L"Matl offset: 0x" << hex << s.header.matlOffset_10 << dec << L"\n";
    ss << L"Model3D offset: 0x" << hex << s.header.model3dOffset_14 << dec << L"\n";
    ss << L"NodeCount: " << s.header.nodeCount_18 << L"\n";
    ss << L"NodeFreeHead: " << s.header.nodeFreeHead_1C << L"\n";
    ss << L"NodeTable offset: 0x" << hex << s.header.nodeTableOffset_20 << dec << L"\n";

    ss << L"\nClassType histogram (top 20)\n";
    ss << L"----------------------------\n";

    vector<pair<unsigned int, unsigned int> > v;
    v.reserve(s.classTypeHistogram.size());
    for (map<unsigned int, unsigned int>::const_iterator it = s.classTypeHistogram.begin();
         it != s.classTypeHistogram.end(); ++it)
        v.push_back(*it);

    sort(v.begin(), v.end(), ClassTypeCountGreater);
    const size_t limit = min<size_t>(20, v.size());
    for (size_t i = 0; i < limit; i++) {
        ss << L"  " << v[i].first << L": " << v[i].second << L"\n";
    }

    if (v.size() > limit)
        ss << L"  (" << (v.size() - limit) << L" more...)\n";

    return ss.str();
}

wstring ReadZbdSummaryText(const wstring &path) {
    Summary s;
    InitSummary(s);
    s.fileSize = GetFileSizeOrZero(path);

    FILE *f = 0;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) {
        const DWORD err = GetLastError();
        wstringstream ss;
        ss << L"Failed to open file. " << Win32LastErrorToString(err);
        return ss.str();
    }

    Header hdr = {0};
    const bool okHdr = ReadExact(f, &hdr, sizeof(hdr));
    if (!okHdr) {
        fclose(f);
        return L"Failed to read ZBD header (0x24 bytes).";
    }

    s.header = hdr;
    s.headerValid = (hdr.magic_00 == kMagic && hdr.version_04 == kVersion);

    // Header-only summary if invalid.
    if (!s.headerValid) {
        fclose(f);
        return FormatSummary(s);
    }

    // Read node records and build a histogram of classType_34.
    if (fseek(f, (long)(hdr.nodeTableOffset_20), SEEK_SET) != 0) {
        fclose(f);
        return L"Failed to seek to node table offset.";
    }

    const unsigned int nodeCount = hdr.nodeCount_18;
    for (unsigned int i = 0; i < nodeCount; i++) {
        NodeRecord32 rec = {0};
        if (!ReadExact(f, &rec, sizeof(rec))) {
            fclose(f);
            return L"Failed to read node table record(s).";
        }
        s.classTypeHistogram[rec.classType_34] += 1;
    }

    fclose(f);
    return FormatSummary(s);
}

struct NodePayloadInfo {
    unsigned int classType = 0;
    unsigned int payloadOff24 = 0;
    unsigned int flags = 0;

    // These fields come from the raw 0xC4 node table record on disk.
    // In the original writer (cls_zbd.c), these are runtime-only pointers/callbacks and are
    // typically cleared.
    unsigned int classDataPtr_38 = 0;
    unsigned int actionCallback_48 = 0;
    int listCountA_54 = 0;
    unsigned int listA_ptr_58 = 0;
    int listCountB_5C = 0;
    unsigned int listB_ptr_60 = 0;
};

struct PayloadSanitySummary {
    unsigned int checkedKnownTypes = 0;
    unsigned int unknownClassType = 0;
    unsigned int rangeFail = 0;
    unsigned int cameraBadRef = 0;
    unsigned int worldBadCounts = 0;
    unsigned int worldBadNodeRefs = 0;
    unsigned int worldNodeRefCheckSkipped = 0;
    unsigned int worldBadChildNodeRefs = 0;
    unsigned int worldChildNodeRefCheckSkipped = 0;
    unsigned int worldVarRangeFail = 0;
    unsigned int worldVarRangeSkipped = 0; // grid walk throttled
    unsigned __int64 worldVarCellsChecked = 0;
    unsigned __int64 worldVarCellsTotal = 0;
    unsigned __int64 worldChildNodeRefsChecked = 0;
    unsigned __int64 worldChildNodeRefsTotal = 0;
    unsigned int lightBadCounts = 0;
    unsigned int lightBadNodeRefs = 0;
    unsigned int lightNodeRefCheckSkipped = 0;
    unsigned int lightVarRangeFail = 0;
    unsigned int soundBadCounts = 0;
    unsigned int soundBadNodeRefs = 0;
    unsigned int soundNodeRefCheckSkipped = 0;
    unsigned int soundVarRangeFail = 0;

    vector<unsigned int> sampleUnknown;
    vector<unsigned int> sampleRangeFail;
    vector<unsigned int> sampleCameraBadRef;
    vector<unsigned int> sampleWorldBad;
    vector<unsigned int> sampleWorldBadNodeRefs;
    vector<unsigned int> sampleWorldNodeRefCheckSkipped;
    vector<unsigned int> sampleWorldBadChildNodeRefs;
    vector<unsigned int> sampleWorldChildNodeRefCheckSkipped;
    vector<unsigned int> sampleWorldVarRangeFail;
    vector<unsigned int> sampleWorldVarRangeSkipped;
    vector<unsigned int> sampleLightBad;
    vector<unsigned int> sampleLightBadNodeRefs;
    vector<unsigned int> sampleLightNodeRefCheckSkipped;
    vector<unsigned int> sampleLightVarRangeFail;
    vector<unsigned int> sampleSoundBad;
    vector<unsigned int> sampleSoundBadNodeRefs;
    vector<unsigned int> sampleSoundNodeRefCheckSkipped;
    vector<unsigned int> sampleSoundVarRangeFail;
};

static bool ReadAt(FILE *f, unsigned __int64 off, void *dst, size_t sz) {
    if (_fseeki64(f, (__int64)(off), SEEK_SET) != 0)
        return false;
    return ReadExact(f, dst, sz);
}

static bool CanAddRange(unsigned __int64 base, unsigned __int64 add, unsigned __int64 fileSize) {
    if (base > fileSize)
        return false;
    if (add > fileSize)
        return false;
    return base <= (fileSize - add);
}

static bool ValidateNodeRefList(FILE *f, unsigned __int64 off, unsigned int count,
                                unsigned int nodeCount, bool &skipped) {
    // Valid NodeRef is -1 or [0,nodeCount). We read the list and check all entries.
    // To avoid excessive allocations, skip very large lists.
    skipped = false;
    if (count == 0)
        return true;
    const unsigned __int64 bytes = (unsigned __int64)(count) * 4ui64;
    if (bytes > kNodeRefListMaxBytes) {
        skipped = true;
        return true;
    }
    if (_fseeki64(f, (__int64)(off), SEEK_SET) != 0)
        return false;

    vector<int> refs;
    refs.resize(count);
    if (!ReadExact(f, &refs[0], (size_t)bytes))
        return false;

    for (unsigned int i = 0; i < count; i++) {
        const int r = refs[i];
        if (!(r == -1 || (r >= 0 && (unsigned int)(r) < nodeCount)))
            return false;
    }
    return true;
}

static unsigned int PayloadSizeForClassType(unsigned int classType) {
    // Sizes confirmed in Binary Ninja: malloc/fread in GameZ_ZBD_ReadNodeClassData.
    switch (classType) {
    case 1:
        return 0x1E8; // camera
    case 2:
        return 0xAC; // world
    case 3:
        return 0xF8; // window
    case 4:
        return 0x1C; // display
    case 5:
        return 0x90; // object3d
    case 6:
        return 0x50; // lod
    case 9:
        return 0xE4; // light
    case 10:
        return 0x94; // sound
    default:
        return 0;
    }
}

static void PrintStrongSamples(wstringstream &ss, const Summary &s,
                               const vector<NodePayloadInfo> &payloadOffsets,
                               const wchar_t *name, const vector<unsigned int> &idxs) {
    if (idxs.empty())
        return;

    ss << L"\n  Strong anomaly samples: " << name << L"\n";
    for (vector<unsigned int>::const_iterator it = idxs.begin(); it != idxs.end(); ++it) {
        const unsigned int idx = *it;
        const NodePayloadInfo &info = payloadOffsets[idx];
        const unsigned __int64 nodeRecOff =
            (unsigned __int64)(s.header.nodeTableOffset_20) +
            (unsigned __int64)(idx) * sizeof(NodeRecord32);
        ss << L"    [" << idx << L"] type=" << info.classType << L" off=0x" << hex
           << info.payloadOff24 << L" nodeRecOff=0x" << nodeRecOff << L" classDataPtr=0x"
           << info.classDataPtr_38 << L" actionCb=0x" << info.actionCallback_48 << dec
           << L" listCountA=" << info.listCountA_54 << L" listA=0x" << hex
           << info.listA_ptr_58 << dec << L" listCountB=" << info.listCountB_5C
           << L" listB=0x" << hex << info.listB_ptr_60 << dec << L"\n";
    }
}

static void PrintPointerFieldSamples(wstringstream &ss, const Summary &s,
                                     const vector<NodePayloadInfo> &payloadOffsets,
                                     const vector<unsigned int> &samplePtrFields) {
    if (samplePtrFields.empty())
        return;

    ss << L"\n  Samples (index, type, payloadOff24, nodeRecOff, classDataPtr_38, actionCb_48, "
          L"listCountA_54, listA_58, listCountB_5C, listB_60)\n";
    for (vector<unsigned int>::const_iterator it = samplePtrFields.begin();
         it != samplePtrFields.end(); ++it) {
        const unsigned int idx = *it;
        const NodePayloadInfo &info = payloadOffsets[idx];
        const unsigned __int64 nodeRecOff =
            (unsigned __int64)(s.header.nodeTableOffset_20) +
            (unsigned __int64)(idx) * sizeof(NodeRecord32);
        ss << L"    [" << idx << L"] type=" << info.classType << L" off=0x" << hex
           << info.payloadOff24 << L" nodeRecOff=0x" << nodeRecOff << L" classDataPtr=0x"
           << hex << info.classDataPtr_38 << L" actionCb=0x" << info.actionCallback_48
           << L" listCountA=" << dec << info.listCountA_54 << L" listA=0x" << hex
           << info.listA_ptr_58 << L" listCountB=" << dec << info.listCountB_5C
           << L" listB=0x" << hex << info.listB_ptr_60 << dec << L"\n";
    }
}

static void PrintOffsetLine(wstringstream &ss, const Summary &s, const wchar_t *name,
                            unsigned int off) {
    ss << L"  " << name << L": 0x" << hex << off << dec;
    if (off < s.fileSize)
        ss << L" (ok)\n";
    else
        ss << L" (out of range)\n";
}

static void PrintPayloadOffsetSamples(wstringstream &ss,
                                      const vector<NodePayloadInfo> &payloadOffsets,
                                      const wchar_t *name,
                                      const vector<unsigned int> &idxs) {
    if (idxs.empty())
        return;

    ss << L"\n  Samples: " << name << L"\n";
    for (vector<unsigned int>::const_iterator it = idxs.begin(); it != idxs.end(); ++it) {
        const unsigned int idx = *it;
        if (idx >= payloadOffsets.size())
            continue;
        const NodePayloadInfo &info = payloadOffsets[idx];
        ss << L"    [" << idx << L"] type=" << info.classType << L" off=0x" << hex
           << info.payloadOff24 << L" flags=0x" << info.flags << dec << L"\n";
    }
}

static wstring
FormatDetailed(const Summary &s, const vector<NodePayloadInfo> &payloadOffsets,
               const map<unsigned int, unsigned int>
                   &payloadOffsetHistogram, // classType -> count of nonzero payload offsets
               const PayloadSanitySummary &payloadSanity) {
    wstringstream ss;
    ss << FormatSummary(s);

    if (!s.headerValid)
        return ss.str();

    ss << L"\nNode payload offsets (flags_C0 & 0x00FFFFFF)\n";
    ss << L"----------------------------------------\n";

    unsigned int minOff = 0xFFFFFFFFu;
    unsigned int maxOff = 0;
    unsigned int nonZero = 0;
    unsigned int outOfRange = 0;
    unsigned int beforeNodeTableEnd = 0;

    const unsigned __int64 nodeTableEnd =
        (unsigned __int64)(s.header.nodeTableOffset_20) +
        (unsigned __int64)(s.header.nodeCount_18) * sizeof(NodeRecord32);

    for (vector<NodePayloadInfo>::const_iterator it = payloadOffsets.begin();
         it != payloadOffsets.end(); ++it) {
        const unsigned int off = it->payloadOff24;
        if (off == 0)
            continue;
        nonZero++;
        minOff = min(minOff, off);
        maxOff = max(maxOff, off);
        if (off >= s.fileSize)
            outOfRange++;
        if ((unsigned __int64)(off) < nodeTableEnd)
            beforeNodeTableEnd++;
    }

    if (nonZero == 0) {
        ss << L"No non-zero payload offsets found in node table.\n";
    } else {
        ss << L"Non-zero offsets: " << nonZero << L" / " << s.header.nodeCount_18 << L"\n";
        ss << L"Min offset: 0x" << hex << minOff << dec << L"\n";
        ss << L"Max offset: 0x" << hex << maxOff << dec << L"\n";
        ss << L"Offsets >= file size: " << outOfRange << L"\n";
        ss << L"Offsets before end of node table (suspicious): " << beforeNodeTableEnd << L"\n";
        ss << L"Node table offset: 0x" << hex << s.header.nodeTableOffset_20 << dec
           << L"\n";
        ss << L"Node table end:    0x" << hex << nodeTableEnd << dec << L"\n";
    }

    // Runtime-only pointer fields present in raw on-disk node records.
    // These are not used by the reader; they should usually be 0 in files written by the original
    // engine.
    unsigned int nonZeroClassDataPtr = 0;
    unsigned int nonZeroActionCb = 0;
    unsigned int nonZeroListA = 0;
    unsigned int nonZeroListB = 0;
    unsigned int nonZeroListA_WithNonPositiveCount = 0;
    unsigned int nonZeroListB_WithNonPositiveCount = 0;
    unsigned int nonZeroClassDataPtr_WithZeroPayloadOff = 0;
    unsigned int nonZeroActionCb_WithZeroPayloadOff = 0;

    // Strong anomaly samples (up to 8 each)
    vector<unsigned int> sampleClassDataPtrWithZeroOff;
    vector<unsigned int> sampleActionCbWithZeroOff;
    vector<unsigned int> sampleListAWithNonPositiveCount;
    vector<unsigned int> sampleListBWithNonPositiveCount;
    sampleClassDataPtrWithZeroOff.reserve(8);
    sampleActionCbWithZeroOff.reserve(8);
    sampleListAWithNonPositiveCount.reserve(8);
    sampleListBWithNonPositiveCount.reserve(8);

    vector<unsigned int> samplePtrFields;
    samplePtrFields.reserve(16);
    for (size_t i = 0; i < payloadOffsets.size(); i++) {
        const NodePayloadInfo &n = payloadOffsets[i];
        bool any = false;
        if (n.classDataPtr_38 != 0) {
            nonZeroClassDataPtr++;
            any = true;
            if (n.payloadOff24 == 0) {
                nonZeroClassDataPtr_WithZeroPayloadOff++;
                if (sampleClassDataPtrWithZeroOff.size() < 8)
                    sampleClassDataPtrWithZeroOff.push_back((unsigned int)(i));
            }
        }
        if (n.actionCallback_48 != 0) {
            nonZeroActionCb++;
            any = true;
            if (n.payloadOff24 == 0) {
                nonZeroActionCb_WithZeroPayloadOff++;
                if (sampleActionCbWithZeroOff.size() < 8)
                    sampleActionCbWithZeroOff.push_back((unsigned int)(i));
            }
        }
        if (n.listA_ptr_58 != 0) {
            nonZeroListA++;
            any = true;
            if (n.listCountA_54 <= 0) {
                nonZeroListA_WithNonPositiveCount++;
                if (sampleListAWithNonPositiveCount.size() < 8)
                    sampleListAWithNonPositiveCount.push_back((unsigned int)(i));
            }
        }
        if (n.listB_ptr_60 != 0) {
            nonZeroListB++;
            any = true;
            if (n.listCountB_5C <= 0) {
                nonZeroListB_WithNonPositiveCount++;
                if (sampleListBWithNonPositiveCount.size() < 8)
                    sampleListBWithNonPositiveCount.push_back((unsigned int)(i));
            }
        }
        if (any && samplePtrFields.size() < 16)
            samplePtrFields.push_back((unsigned int)(i));
    }

    ss << L"\nOn-disk runtime-pointer fields (informational)\n";
    ss << L"-------------------------------------------\n";
    ss << L"classDataPtr_38 non-zero: " << nonZeroClassDataPtr << L" (of which payloadOff24==0: "
       << nonZeroClassDataPtr_WithZeroPayloadOff << L")\n";
    ss << L"actionCallback_48 non-zero: " << nonZeroActionCb << L" (of which payloadOff24==0: "
       << nonZeroActionCb_WithZeroPayloadOff << L")\n";
    ss << L"listA_ptr_58 non-zero: " << nonZeroListA << L" (of which count<=0: "
       << nonZeroListA_WithNonPositiveCount << L")\n";
    ss << L"listB_ptr_60 non-zero: " << nonZeroListB << L" (of which count<=0: "
       << nonZeroListB_WithNonPositiveCount << L")\n";

    const unsigned int strongTotal =
        nonZeroClassDataPtr_WithZeroPayloadOff + nonZeroActionCb_WithZeroPayloadOff +
        nonZeroListA_WithNonPositiveCount + nonZeroListB_WithNonPositiveCount;

    ss << L"Strong anomaly total (sum of predicate counts): " << strongTotal << L"\n";

    if (strongTotal != 0)
        ss << L"WARNING: strong anomalies present in on-disk node records (unexpected runtime "
              L"pointers/callbacks).\n";

    PrintStrongSamples(ss, s, payloadOffsets, L"classDataPtr_38 != 0 && payloadOff24 == 0", sampleClassDataPtrWithZeroOff);
    PrintStrongSamples(ss, s, payloadOffsets, L"actionCallback_48 != 0 && payloadOff24 == 0", sampleActionCbWithZeroOff);
    PrintStrongSamples(ss, s, payloadOffsets, L"listA_ptr_58 != 0 && listCountA_54 <= 0", sampleListAWithNonPositiveCount);
    PrintStrongSamples(ss, s, payloadOffsets, L"listB_ptr_60 != 0 && listCountB_5C <= 0", sampleListBWithNonPositiveCount);

    PrintPointerFieldSamples(ss, s, payloadOffsets, samplePtrFields);

    ss << L"\nSection offset sanity\n";
    ss << L"---------------------\n";

    PrintOffsetLine(ss, s, L"TexDir offset", s.header.texDirOffset_0C);
    PrintOffsetLine(ss, s, L"Matl offset", s.header.matlOffset_10);
    PrintOffsetLine(ss, s, L"Model3D offset", s.header.model3dOffset_14);
    PrintOffsetLine(ss, s, L"NodeTable offset", s.header.nodeTableOffset_20);

    const bool monotonic = (s.header.texDirOffset_0C < s.header.matlOffset_10) &&
                           (s.header.matlOffset_10 < s.header.model3dOffset_14) &&
                           (s.header.model3dOffset_14 < s.header.nodeTableOffset_20);

    if (!monotonic)
        ss << L"  WARNING: section offsets are not strictly increasing.\n";

    if (nodeTableEnd > s.fileSize)
        ss << L"  WARNING: node table end is beyond file size.\n";

    ss << L"\nPayload offset sanity vs sections\n";
    ss << L"--------------------------------\n";

    unsigned int payloadBeforeTexDir = 0;
    unsigned int payloadInTexDir = 0;
    unsigned int payloadInMatl = 0;
    unsigned int payloadInModel3D = 0;
    unsigned int payloadInNodeTable = 0;
    unsigned int payloadInExpectedRegion = 0;
    unsigned int payloadOutOfRange2 = 0;

    vector<unsigned int> sampleBeforeTexDir;
    vector<unsigned int> sampleInTexDir;
    vector<unsigned int> sampleInMatl;
    vector<unsigned int> sampleInModel3D;
    vector<unsigned int> sampleInNodeTable;
    vector<unsigned int> sampleOutOfRange;
    vector<unsigned int> sampleExpectedRegion;
    sampleBeforeTexDir.reserve(16);
    sampleInTexDir.reserve(16);
    sampleInMatl.reserve(16);
    sampleInModel3D.reserve(16);
    sampleInNodeTable.reserve(16);
    sampleOutOfRange.reserve(16);
    sampleExpectedRegion.reserve(8);

    for (size_t idx = 0; idx < payloadOffsets.size(); idx++) {
        const NodePayloadInfo &kv = payloadOffsets[idx];
        const unsigned int off32 = kv.payloadOff24;
        if (off32 == 0)
            continue;

        const unsigned __int64 off = (unsigned __int64)(off32);
        if (off >= s.fileSize) {
            payloadOutOfRange2++;
            if (sampleOutOfRange.size() < 16)
                sampleOutOfRange.push_back((unsigned int)(idx));
            continue;
        }

        if (off < (unsigned __int64)(s.header.texDirOffset_0C)) {
            payloadBeforeTexDir++;
            if (sampleBeforeTexDir.size() < 16)
                sampleBeforeTexDir.push_back((unsigned int)(idx));
            continue;
        }

        if (monotonic) {
            if (off < (unsigned __int64)(s.header.matlOffset_10)) {
                payloadInTexDir++;
                if (sampleInTexDir.size() < 16)
                    sampleInTexDir.push_back((unsigned int)(idx));
            } else if (off < (unsigned __int64)(s.header.model3dOffset_14)) {
                payloadInMatl++;
                if (sampleInMatl.size() < 16)
                    sampleInMatl.push_back((unsigned int)(idx));
            } else if (off < (unsigned __int64)(s.header.nodeTableOffset_20)) {
                payloadInModel3D++;
                if (sampleInModel3D.size() < 16)
                    sampleInModel3D.push_back((unsigned int)(idx));
            } else if (off < nodeTableEnd) {
                payloadInNodeTable++;
                if (sampleInNodeTable.size() < 16)
                    sampleInNodeTable.push_back((unsigned int)(idx));
            } else {
                payloadInExpectedRegion++;
                if (sampleExpectedRegion.size() < 8)
                    sampleExpectedRegion.push_back((unsigned int)(idx));
            }
        } else {
            // Without monotonic section offsets we can't classify reliably; only node-table
            // containment is meaningful.
            if (off < nodeTableEnd) {
                payloadInNodeTable++;
                if (sampleInNodeTable.size() < 16)
                    sampleInNodeTable.push_back((unsigned int)(idx));
            } else {
                payloadInExpectedRegion++;
                if (sampleExpectedRegion.size() < 8)
                    sampleExpectedRegion.push_back((unsigned int)(idx));
            }
        }
    }

    ss << L"  In expected region (>= nodeTableEnd): " << payloadInExpectedRegion << L"\n";
    ss << L"  Inside node table [nodeTableOffset,nodeTableEnd): " << payloadInNodeTable << L"\n";
    ss << L"  Before TexDir offset: " << payloadBeforeTexDir << L"\n";
    if (monotonic) {
        ss << L"  Inside TexDir section: " << payloadInTexDir << L"\n";
        ss << L"  Inside Matl section: " << payloadInMatl << L"\n";
        ss << L"  Inside Model3D section: " << payloadInModel3D << L"\n";
    }
    ss << L"  Out of range (>= file size): " << payloadOutOfRange2 << L"\n";

    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Out of range", sampleOutOfRange);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Before TexDir", sampleBeforeTexDir);
    if (monotonic) {
        PrintPayloadOffsetSamples(ss, payloadOffsets, L"Inside TexDir", sampleInTexDir);
        PrintPayloadOffsetSamples(ss, payloadOffsets, L"Inside Matl", sampleInMatl);
        PrintPayloadOffsetSamples(ss, payloadOffsets, L"Inside Model3D", sampleInModel3D);
    }
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Inside node table", sampleInNodeTable);

    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Expected region (>= nodeTableEnd)",
                              sampleExpectedRegion);

    ss << L"\nPayload header sanity (expected-region payloads, known class types)\n";
    ss << L"---------------------------------------------------------------\n";

    ss << L"Validation caps (perf guards)\n";
    ss << L"  NodeRef list max bytes: 0x" << hex << kNodeRefListMaxBytes << dec << L"\n";
    ss << L"  World max cells walk: " << kWorldMaxCellsWalk << L"\n";
    ss << L"  World max total child NodeRefs checked: " << kWorldMaxTotalChildRefsToCheck << L"\n";
    ss << L"  World child NodeRef chunk: " << kWorldChildRefChunk << L"\n\n";

    ss << L"Checked known-type payload headers: " << payloadSanity.checkedKnownTypes << L"\n";
    ss << L"Unknown classType payloads (size mapping missing): " << payloadSanity.unknownClassType
       << L"\n";
    ss << L"Range failures (off+size > file size): " << payloadSanity.rangeFail << L"\n";
    ss << L"Camera bad NodeRef indices (refs_00[0..3]): " << payloadSanity.cameraBadRef << L"\n";
    ss << L"World suspicious counts (cols/rows/listCounts): " << payloadSanity.worldBadCounts
       << L"\n";
    ss << L"World bad NodeRef indices (list1/list2): " << payloadSanity.worldBadNodeRefs << L"\n";
    ss << L"World NodeRef checks skipped (list too large): "
       << payloadSanity.worldNodeRefCheckSkipped << L"\n";
    ss << L"World bad child NodeRef indices (area cell lists): "
       << payloadSanity.worldBadChildNodeRefs << L"\n";
    ss << L"World child NodeRef checks skipped (cap/too large): "
       << payloadSanity.worldChildNodeRefCheckSkipped << L"\n";
    ss << L"World variable-range failures (lists/area grid): " << payloadSanity.worldVarRangeFail
       << L"\n";
    ss << L"World variable-range skipped (grid walk throttled): "
       << payloadSanity.worldVarRangeSkipped << L"\n";
    ss << L"World variable-range cell coverage: " << payloadSanity.worldVarCellsChecked << L" / "
       << payloadSanity.worldVarCellsTotal << L"\n";
    ss << L"World child NodeRef coverage (walked cells only): "
       << payloadSanity.worldChildNodeRefsChecked << L" / " << payloadSanity.worldChildNodeRefsTotal
       << L"\n";
    ss << L"Light suspicious attachedCount: " << payloadSanity.lightBadCounts << L"\n";
    ss << L"Light bad NodeRef indices (attached list): " << payloadSanity.lightBadNodeRefs << L"\n";
    ss << L"Light NodeRef checks skipped (list too large): "
       << payloadSanity.lightNodeRefCheckSkipped << L"\n";
    ss << L"Light variable-range failures (attached list): " << payloadSanity.lightVarRangeFail
       << L"\n";
    ss << L"Sound suspicious attachedCount: " << payloadSanity.soundBadCounts << L"\n";
    ss << L"Sound bad NodeRef indices (attached list): " << payloadSanity.soundBadNodeRefs << L"\n";
    ss << L"Sound NodeRef checks skipped (list too large): "
       << payloadSanity.soundNodeRefCheckSkipped << L"\n";
    ss << L"Sound variable-range failures (attached list): " << payloadSanity.soundVarRangeFail
       << L"\n";

    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Unknown classType", payloadSanity.sampleUnknown);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Range fail", payloadSanity.sampleRangeFail);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Camera bad ref", payloadSanity.sampleCameraBadRef);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World bad counts", payloadSanity.sampleWorldBad);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World bad NodeRefs", payloadSanity.sampleWorldBadNodeRefs);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World NodeRef check skipped",
                        payloadSanity.sampleWorldNodeRefCheckSkipped);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World bad child NodeRefs", payloadSanity.sampleWorldBadChildNodeRefs);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World child NodeRef check skipped",
                        payloadSanity.sampleWorldChildNodeRefCheckSkipped);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World variable-range fail", payloadSanity.sampleWorldVarRangeFail);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"World variable-range skipped", payloadSanity.sampleWorldVarRangeSkipped);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Light bad count", payloadSanity.sampleLightBad);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Light bad NodeRefs", payloadSanity.sampleLightBadNodeRefs);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Light NodeRef check skipped",
                        payloadSanity.sampleLightNodeRefCheckSkipped);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Light variable-range fail", payloadSanity.sampleLightVarRangeFail);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Sound bad count", payloadSanity.sampleSoundBad);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Sound bad NodeRefs", payloadSanity.sampleSoundBadNodeRefs);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Sound NodeRef check skipped",
                        payloadSanity.sampleSoundNodeRefCheckSkipped);
    PrintPayloadOffsetSamples(ss, payloadOffsets, L"Sound variable-range fail", payloadSanity.sampleSoundVarRangeFail);

    ss << L"\nClassType -> nodes with non-zero payload offset\n";
    ss << L"----------------------------------------------\n";
    vector<pair<unsigned int, unsigned int> > v;
    v.reserve(payloadOffsetHistogram.size());
    for (map<unsigned int, unsigned int>::const_iterator it = payloadOffsetHistogram.begin();
         it != payloadOffsetHistogram.end(); ++it)
        v.push_back(*it);
    sort(v.begin(), v.end(), ClassTypeCountGreater);
    for (vector<pair<unsigned int, unsigned int> >::const_iterator it = v.begin(); it != v.end();
         ++it)
        ss << L"  " << it->first << L": " << it->second << L"\n";

    ss << L"\nFirst 32 nodes (index, classType_34, payloadOff24, flags_C0)\n";
    ss << L"-----------------------------------------------------------\n";
    const size_t limit = min<size_t>(32, payloadOffsets.size());
    for (size_t i = 0; i < limit; i++) {
        const NodePayloadInfo &info = payloadOffsets[i];
        ss << L"  [" << i << L"] type=" << info.classType << L" off=0x" << hex
           << info.payloadOff24 << L" flags=0x" << info.flags << dec << L"\n";
    }

    ss << L"\nNotes\n";
    ss << L"-----\n";
    ss << L"- In the original x86 engine, node table records are 0xC4 bytes.\n";
    ss << L"- Writer stores payload file offset in flags_C0 low 24 bits (mask 0x00FFFFFF).\n";
    ss << L"- Reader also uses bit 0x01000000 in flags_C0 as a runtime 'loaded' bit.\n";

    return ss.str();
}

wstring ReadZbdDetailedText(const wstring &path) {
    Summary s;
    InitSummary(s);
    s.fileSize = GetFileSizeOrZero(path);

    FILE *f = 0;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) {
        const DWORD err = GetLastError();
        wstringstream ss;
        ss << L"Failed to open file. " << Win32LastErrorToString(err);
        return ss.str();
    }

    Header hdr = {0};
    if (!ReadExact(f, &hdr, sizeof(hdr))) {
        fclose(f);
        return L"Failed to read ZBD header (0x24 bytes).";
    }

    s.header = hdr;
    s.headerValid = (hdr.magic_00 == kMagic && hdr.version_04 == kVersion);

    if (!s.headerValid) {
        fclose(f);
        return FormatDetailed(s, {}, {}, PayloadSanitySummary{});
    }

    if (fseek(f, (long)(hdr.nodeTableOffset_20), SEEK_SET) != 0) {
        fclose(f);
        return L"Failed to seek to node table offset.";
    }

    vector<NodePayloadInfo> payloadOffsets;
    payloadOffsets.reserve(hdr.nodeCount_18);

    map<unsigned int, unsigned int> payloadOffsetHistogram;

    for (unsigned int i = 0; i < hdr.nodeCount_18; i++) {
        NodeRecord32 rec = {0};
        if (!ReadExact(f, &rec, sizeof(rec))) {
            fclose(f);
            return L"Failed to read node table record(s).";
        }

        s.classTypeHistogram[rec.classType_34] += 1;
        const unsigned int payloadOff = (rec.flags_C0 & 0x00FFFFFFu);
        NodePayloadInfo info;
        info.classType = rec.classType_34;
        info.payloadOff24 = payloadOff;
        info.flags = rec.flags_C0;
        info.classDataPtr_38 = rec.classDataPtr_38;
        info.actionCallback_48 = rec.actionCallback_48;
        info.listCountA_54 = rec.listCountA_54;
        info.listA_ptr_58 = rec.listA_58;
        info.listCountB_5C = rec.listCountB_5C;
        info.listB_ptr_60 = rec.listB_60;
        payloadOffsets.push_back(info);
        if (payloadOff != 0)
            payloadOffsetHistogram[rec.classType_34] += 1;
    }

    // Payload header sanity pass. This does NOT parse variable-length lists; it only validates
    // fixed-size payload headers for a subset of known class types.
    PayloadSanitySummary payloadSanity = {0};
    payloadSanity.sampleUnknown.reserve(8);
    payloadSanity.sampleRangeFail.reserve(8);
    payloadSanity.sampleCameraBadRef.reserve(8);
    payloadSanity.sampleWorldBad.reserve(8);
    payloadSanity.sampleWorldBadNodeRefs.reserve(8);
    payloadSanity.sampleWorldNodeRefCheckSkipped.reserve(8);
    payloadSanity.sampleWorldBadChildNodeRefs.reserve(8);
    payloadSanity.sampleWorldChildNodeRefCheckSkipped.reserve(8);
    payloadSanity.sampleWorldVarRangeFail.reserve(8);
    payloadSanity.sampleWorldVarRangeSkipped.reserve(8);
    payloadSanity.sampleLightBad.reserve(8);
    payloadSanity.sampleLightBadNodeRefs.reserve(8);
    payloadSanity.sampleLightNodeRefCheckSkipped.reserve(8);
    payloadSanity.sampleLightVarRangeFail.reserve(8);
    payloadSanity.sampleSoundBad.reserve(8);
    payloadSanity.sampleSoundBadNodeRefs.reserve(8);
    payloadSanity.sampleSoundNodeRefCheckSkipped.reserve(8);
    payloadSanity.sampleSoundVarRangeFail.reserve(8);

    const unsigned __int64 nodeTableEnd =
        (unsigned __int64)(hdr.nodeTableOffset_20) +
        (unsigned __int64)(hdr.nodeCount_18) * sizeof(NodeRecord32);

    for (unsigned int i = 0; i < hdr.nodeCount_18; i++) {
        const NodePayloadInfo &info = payloadOffsets[i];
        const unsigned int off32 = info.payloadOff24;
        if (off32 == 0)
            continue;

        const unsigned __int64 off = (unsigned __int64)(off32);
        if (off < nodeTableEnd || off >= s.fileSize)
            continue; // already covered by offset sanity buckets

        const unsigned int payloadSize = PayloadSizeForClassType(info.classType);
        if (payloadSize == 0) {
            payloadSanity.unknownClassType++;
            if (payloadSanity.sampleUnknown.size() < 8)
                payloadSanity.sampleUnknown.push_back(i);
            continue;
        }

        if (off + payloadSize > s.fileSize) {
            payloadSanity.rangeFail++;
            if (payloadSanity.sampleRangeFail.size() < 8)
                payloadSanity.sampleRangeFail.push_back(i);
            continue;
        }

        payloadSanity.checkedKnownTypes++;

        // Class-specific header checks.
        if (info.classType == 1) {
            // Camera: refs_00[4] at offset 0x00.
            int refs[4] = {0};
            if (ReadAt(f, off, refs, sizeof(refs))) {
                bool bad = false;
                for (int k = 0; k < 4; k++) {
                    const int r = refs[k];
                    if (!(r == -1 || (r >= 0 && (unsigned int)(r) < hdr.nodeCount_18)))
                        bad = true;
                }
                if (bad) {
                    payloadSanity.cameraBadRef++;
                    if (payloadSanity.sampleCameraBadRef.size() < 8)
                        payloadSanity.sampleCameraBadRef.push_back(i);
                }
            }
        } else if (info.classType == 2) {
            // World: validate key counts.
            unsigned char buf[0xAC] = {0};
            if (ReadAt(f, off, buf, sizeof(buf))) {
                const unsigned int cols = ReadU32FromBuffer(buf, 0x78);
                const unsigned int rows = ReadU32FromBuffer(buf, 0x7C);
                const int list1 = ReadI32FromBuffer(buf, 0x90);
                const int list2 = ReadI32FromBuffer(buf, 0x9C);

                bool bad = false;
                if (cols == 0 || rows == 0 || cols > 8192 || rows > 8192)
                    bad = true;
                if (list1 < 0 || list2 < 0)
                    bad = true;
                if (list1 > (int)(hdr.nodeCount_18) ||
                    list2 > (int)(hdr.nodeCount_18))
                    bad = true;

                if (bad) {
                    payloadSanity.worldBadCounts++;
                    if (payloadSanity.sampleWorldBad.size() < 8)
                        payloadSanity.sampleWorldBad.push_back(i);
                }

                // Validate NodeRef indices in the two world-level lists (if sizes are reasonable).
                if (!bad) {
                    const unsigned __int64 list1Off = off + 0xAC;
                    const unsigned __int64 list2Off =
                        list1Off + (unsigned __int64)(list1) * 4ui64;
                    bool skipped1 = false;
                    bool skipped2 = false;
                    bool ok1 = ValidateNodeRefList(f, list1Off, (unsigned int)(list1),
                                                   hdr.nodeCount_18, skipped1);
                    bool ok2 = ValidateNodeRefList(f, list2Off, (unsigned int)(list2),
                                                   hdr.nodeCount_18, skipped2);
                    if (skipped1 || skipped2) {
                        payloadSanity.worldNodeRefCheckSkipped++;
                        if (payloadSanity.sampleWorldNodeRefCheckSkipped.size() < 8)
                            payloadSanity.sampleWorldNodeRefCheckSkipped.push_back(i);
                    }
                    if (!ok1 || !ok2) {
                        payloadSanity.worldBadNodeRefs++;
                        if (payloadSanity.sampleWorldBadNodeRefs.size() < 8)
                            payloadSanity.sampleWorldBadNodeRefs.push_back(i);
                    }
                }

                // Variable-length range validation (lists + area grid + per-cell child lists).
                // This does not decode NodeRefs; it only validates that the file has enough bytes.
                if (!bad) {
                    bool vrBad = false;
                    unsigned __int64 cur = off + 0xAC;
                    const unsigned __int64 fileSize = s.fileSize;

                    // Total cap for child NodeRef validation across all walked cells (perf guard).
                    const unsigned __int64 kMaxTotalChildRefsToCheck = kWorldMaxTotalChildRefsToCheck;

                    const unsigned __int64 list1Bytes = (unsigned __int64)(list1) * 4ui64;
                    const unsigned __int64 list2Bytes = (unsigned __int64)(list2) * 4ui64;

                    if (!CanAddRange(cur, list1Bytes, fileSize))
                        vrBad = true;
                    else
                        cur += list1Bytes;

                    if (!vrBad) {
                        if (!CanAddRange(cur, list2Bytes, fileSize))
                            vrBad = true;
                        else
                            cur += list2Bytes;
                    }

                    if (!vrBad) {
                        const unsigned __int64 cells =
                            (unsigned __int64)(cols) * (unsigned __int64)(rows);
                        if (cells > 0x1000000ui64)
                            vrBad = true; // guard absurd multiplication

                        payloadSanity.worldVarCellsTotal += cells;

                        // Throttle very large grids to keep reporting fast.
                        const unsigned __int64 cellsToWalk =
                            (cells > kWorldMaxCellsWalk) ? kWorldMaxCellsWalk : cells;
                        if (cells > kWorldMaxCellsWalk) {
                            payloadSanity.worldVarRangeSkipped++;
                            if (payloadSanity.sampleWorldVarRangeSkipped.size() < 8)
                                payloadSanity.sampleWorldVarRangeSkipped.push_back(i);
                        }

                        // Switch to sequential reads: seek once then ReadExact in the loop.
                        if (_fseeki64(f, (__int64)(cur), SEEK_SET) != 0)
                            vrBad = true;

                        for (unsigned __int64 c = 0; !vrBad && c < cellsToWalk; c++) {
                            if (!CanAddRange(cur, 0x40, fileSize)) {
                                vrBad = true;
                                break;
                            }

                            unsigned char cellBuf[0x40] = {0};
                            if (!ReadExact(f, cellBuf, sizeof(cellBuf))) {
                                vrBad = true;
                                break;
                            }

                            // Child count is a 16-bit value at offset 0x3A.
                            short childCount = 0;
                            memcpy(&childCount, cellBuf + 0x3A, sizeof(childCount));
                            if (childCount < 0)
                                childCount = 0;

                            const unsigned int childCountU =
                                (unsigned int)(childCount);

                            cur += 0x40;
                            payloadSanity.worldVarCellsChecked++;

                            const unsigned __int64 childBytes =
                                (unsigned __int64)(childCountU) * 4ui64;
                            if (!CanAddRange(cur, childBytes, fileSize)) {
                                vrBad = true;
                                break;
                            }

                            // Validate child NodeRef indices for walked cells, with strict perf
                            // caps.
                            if (childCountU != 0) {
                                payloadSanity.worldChildNodeRefsTotal += childCountU;

                                // Skip validation if list is huge or if we hit the global cap.
                                const bool tooLarge = (childBytes > kNodeRefListMaxBytes);
                                const bool capExceeded = (payloadSanity.worldChildNodeRefsChecked >=
                                                          kMaxTotalChildRefsToCheck);
                                if (tooLarge || capExceeded) {
                                    payloadSanity.worldChildNodeRefCheckSkipped++;
                                    if (payloadSanity.sampleWorldChildNodeRefCheckSkipped.size() <
                                        8)
                                        payloadSanity.sampleWorldChildNodeRefCheckSkipped.push_back(
                                            i);

                                    if (_fseeki64(f, (__int64)(childBytes), SEEK_CUR) !=
                                        0) {
                                        vrBad = true;
                                        break;
                                    }
                                } else {
                                    unsigned int toCheck = childCountU;
                                    const unsigned __int64 remaining =
                                        kMaxTotalChildRefsToCheck -
                                        payloadSanity.worldChildNodeRefsChecked;
                                    if ((unsigned __int64)(toCheck) > remaining)
                                        toCheck = (unsigned int)(remaining);

                                    unsigned int toSkip = childCountU - toCheck;
                                    bool badChild = false;

                                    // Chunked sequential read.
                                    while (!vrBad && toCheck != 0) {
                                        const unsigned int chunk = (toCheck > kWorldChildRefChunk)
                                                                        ? kWorldChildRefChunk
                                                                        : toCheck;
                                        int bufRefs[kWorldChildRefChunk] = {0};
                                        if (!ReadExact(f, bufRefs, (size_t)chunk * 4)) {
                                            vrBad = true;
                                            break;
                                        }
                                        for (unsigned int k = 0; k < chunk; k++) {
                                            const int r = bufRefs[k];
                                            if (!(r == -1 || (r >= 0 && (unsigned int)(
                                                                            r) < hdr.nodeCount_18)))
                                                badChild = true;
                                        }
                                        payloadSanity.worldChildNodeRefsChecked += chunk;
                                        toCheck -= chunk;
                                    }

                                    if (badChild) {
                                        payloadSanity.worldBadChildNodeRefs++;
                                        if (payloadSanity.sampleWorldBadChildNodeRefs.size() < 8)
                                            payloadSanity.sampleWorldBadChildNodeRefs.push_back(i);
                                    }

                                    if (!vrBad && toSkip != 0) {
                                        payloadSanity.worldChildNodeRefCheckSkipped++;
                                        if (payloadSanity.sampleWorldChildNodeRefCheckSkipped
                                                .size() < 8)
                                            payloadSanity.sampleWorldChildNodeRefCheckSkipped
                                                .push_back(i);

                                        const unsigned __int64 skipBytes =
                                            (unsigned __int64)(toSkip) * 4ui64;
                                        if (_fseeki64(f, (__int64)(skipBytes),
                                                      SEEK_CUR) != 0) {
                                            vrBad = true;
                                            break;
                                        }
                                    }
                                }
                            }

                            cur += childBytes;
                        }

                        // If we throttled the walk, we intentionally did not validate the
                        // remainder.
                    }

                    if (vrBad) {
                        payloadSanity.worldVarRangeFail++;
                        if (payloadSanity.sampleWorldVarRangeFail.size() < 8)
                            payloadSanity.sampleWorldVarRangeFail.push_back(i);
                    }
                }
            }
        } else if (info.classType == 9) {
            // Light: attachedCount at 0xDC.
            unsigned char buf[0xE0] = {0};
            if (ReadAt(f, off, buf, sizeof(buf))) {
                int count = 0;
                memcpy(&count, buf + 0xDC, 4);
                if (count < 0 || count > (int)(hdr.nodeCount_18)) {
                    payloadSanity.lightBadCounts++;
                    if (payloadSanity.sampleLightBad.size() < 8)
                        payloadSanity.sampleLightBad.push_back(i);
                }

                if (count >= 0) {
                    const unsigned __int64 cur = off + 0xE4;
                    const unsigned __int64 bytes = (unsigned __int64)(count) * 4ui64;
                    if (!CanAddRange(cur, bytes, s.fileSize)) {
                        payloadSanity.lightVarRangeFail++;
                        if (payloadSanity.sampleLightVarRangeFail.size() < 8)
                            payloadSanity.sampleLightVarRangeFail.push_back(i);
                    }

                    // Validate NodeRef indices in attached list.
                    bool skipped = false;
                    const bool okRefs = ValidateNodeRefList(
                        f, cur, (unsigned int)(count), hdr.nodeCount_18, skipped);
                    if (skipped) {
                        payloadSanity.lightNodeRefCheckSkipped++;
                        if (payloadSanity.sampleLightNodeRefCheckSkipped.size() < 8)
                            payloadSanity.sampleLightNodeRefCheckSkipped.push_back(i);
                    }
                    if (!okRefs) {
                        payloadSanity.lightBadNodeRefs++;
                        if (payloadSanity.sampleLightBadNodeRefs.size() < 8)
                            payloadSanity.sampleLightBadNodeRefs.push_back(i);
                    }
                }
            }
        } else if (info.classType == 10) {
            // Sound: attachedCount at 0x8C.
            unsigned char buf[0x90] = {0};
            if (ReadAt(f, off, buf, sizeof(buf))) {
                int count = 0;
                memcpy(&count, buf + 0x8C, 4);
                if (count < 0 || count > (int)(hdr.nodeCount_18)) {
                    payloadSanity.soundBadCounts++;
                    if (payloadSanity.sampleSoundBad.size() < 8)
                        payloadSanity.sampleSoundBad.push_back(i);
                }

                if (count >= 0) {
                    const unsigned __int64 cur = off + 0x94;
                    const unsigned __int64 bytes = (unsigned __int64)(count) * 4ui64;
                    if (!CanAddRange(cur, bytes, s.fileSize)) {
                        payloadSanity.soundVarRangeFail++;
                        if (payloadSanity.sampleSoundVarRangeFail.size() < 8)
                            payloadSanity.sampleSoundVarRangeFail.push_back(i);
                    }

                    // Validate NodeRef indices in attached list.
                    bool skipped = false;
                    const bool okRefs = ValidateNodeRefList(
                        f, cur, (unsigned int)(count), hdr.nodeCount_18, skipped);
                    if (skipped) {
                        payloadSanity.soundNodeRefCheckSkipped++;
                        if (payloadSanity.sampleSoundNodeRefCheckSkipped.size() < 8)
                            payloadSanity.sampleSoundNodeRefCheckSkipped.push_back(i);
                    }
                    if (!okRefs) {
                        payloadSanity.soundBadNodeRefs++;
                        if (payloadSanity.sampleSoundBadNodeRefs.size() < 8)
                            payloadSanity.sampleSoundBadNodeRefs.push_back(i);
                    }
                }
            }
        }
    }

    fclose(f);
    return FormatDetailed(s, payloadOffsets, payloadOffsetHistogram, payloadSanity);
}

wstring ReadZbdNodesCsvText(const wstring &path) {
    const unsigned __int64 fileSize = GetFileSizeOrZero(path);

    FILE *f = 0;
    if (_wfopen_s(&f, path.c_str(), L"rb") != 0 || !f) {
        const DWORD err = GetLastError();
        wstringstream ss;
        ss << L"Failed to open file. " << Win32LastErrorToString(err);
        return ss.str();
    }

    Header hdr = {0};
    if (!ReadExact(f, &hdr, sizeof(hdr))) {
        fclose(f);
        return L"Failed to read ZBD header (0x24 bytes).";
    }

    if (!(hdr.magic_00 == kMagic && hdr.version_04 == kVersion)) {
        fclose(f);
        return L"Invalid ZBD header (magic/version mismatch).";
    }

    if (fseek(f, (long)(hdr.nodeTableOffset_20), SEEK_SET) != 0) {
        fclose(f);
        return L"Failed to seek to node table offset.";
    }

    wstringstream ss;
    ss << L"index,nameHex,classType,payloadOff24,flags,nodeRecOff,classDataPtr_38,actionCallback_"
          L"48,listCountA_54,listA_ptr_58,listCountB_5C,listB_ptr_60\n";

    for (unsigned int i = 0; i < hdr.nodeCount_18; i++) {
        NodeRecord32 rec = {0};
        if (!ReadExact(f, &rec, sizeof(rec))) {
            fclose(f);
            return L"Failed to read node table record(s).";
        }

        const unsigned int payloadOff = (rec.flags_C0 & 0x00FFFFFFu);
        const unsigned __int64 nodeRecOff = (unsigned __int64)(hdr.nodeTableOffset_20) +
                                         (unsigned __int64)(i) * sizeof(NodeRecord32);

        const wstring nameHex = BytesToHex(rec.name_00, sizeof(rec.name_00));

        // Hex for offsets/pointers/flags to match reverse engineering workflows.
        ss << i << L"," << nameHex << L"," << rec.classType_34 << L",0x" << hex << payloadOff
           << L",0x" << hex << rec.flags_C0 << L",0x" << hex << nodeRecOff << L",0x"
           << hex << rec.classDataPtr_38 << L",0x" << hex << rec.actionCallback_48
           << dec << L"," << rec.listCountA_54 << L",0x" << hex << rec.listA_58
           << dec << L"," << rec.listCountB_5C << L",0x" << hex << rec.listB_60
           << dec << L"\n";
    }

    ss << L"# fileSize=0x" << hex << fileSize << dec << L"\n";

    fclose(f);
    return ss.str();
}
} // namespace zbd
