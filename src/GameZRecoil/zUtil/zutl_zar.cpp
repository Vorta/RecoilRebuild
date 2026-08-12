#include "GameZRecoil/zReader/zreader.h"

#include "GameZRecoil/zError/zerr.h"

#include <windows.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Purpose: store the CRT basename component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitFileNameBuf[0x100] = {0};
/**
 * Purpose: store the CRT extension component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitExtBuf[0x100] = {0};
/**
 * Purpose: store joined search-directory and filename probes.
 */
extern "C" char g_zRdr_PathJoinBuf[0x100] = {0};
/**
 * Purpose: store the resolved path returned from ZRDR search-path lookup.
 */
extern "C" char g_zRdr_ResolvedPathBuf[0x100] = {0};
/**
 * Purpose: store the CRT directory component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitDirBuf[0x100] = {0};
/**
 * Purpose: store the CRT drive component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitDriveBuf[4] = {0};
/**
 * Purpose: store the temporary ZRDR search-path list used by scoped lookups.
 */
extern "C" zArchiveList *g_zRdr_ScratchSearchPathList = 0;
/**
 * Purpose: store the odometer digits for active wildcard path expansion.
 */
extern "C" int g_zUtil_ZRDR_WildcardDigits[5] = {0};
/**
 * Purpose: store the active in-place wildcard path string.
 */
extern "C" char *g_zUtil_ZRDR_WildcardPath = 0;
/**
 * Purpose: store the number of active wildcard placeholders, capped at five.
 */
extern "C" int g_zUtil_ZRDR_WildcardStarCount = 0;
/**
 * Storage note: preserves the reserved slot in the ZRDR wildcard state.
 * Purpose: preserve the unreferenced zero-initialized dword between the
 * wildcard count and pointer array proven by BN.
 */
extern "C" int g_zUtil_ZRDR_WildcardReserved = 0;
/**
 * Purpose: store pointers to wildcard placeholder bytes inside the active path.
 */
extern "C" char *g_zUtil_ZRDR_WildcardStarPtrs[5] = {0};
/**
 * Purpose: delimit semicolon-separated ZRDR search paths for CRT tokenization.
 */
extern "C" char g_zRdr_PathDelimStr[2] = ";";
/**
 * Purpose: format a matched ZRDR search directory with the split filename and extension.
 */
extern "C" char g_zUtil_ZarPathJoinFmt[0x8] = "%s\\%s%s";

namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zreader-fileexists
 * @recoil-artifact defines .text recoil:function:0x4a5c20: zReader::FileExists.
 * Uses the imported CRT `_access` provider and returns a 1/0 existence flag.
 * Purpose: test whether a path exists for ZRDR file lookup.
 */
int __fastcall FileExists(
    const char *path
) {
    const int accessResult = _access(
        path,
        0
    );
    return accessResult == 0;
}
} // namespace zReader


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zreader-fileexists-wrapper
 * @recoil-artifact defines .text recoil:function:0x4a5c40: zReader_FileExists_Wrapper.
 * Purpose: expose zReader::FileExists through the original wrapper entry point.
 */
extern "C" int __fastcall zReader_FileExists_Wrapper(
    const char *path
) {
    return zReader::FileExists(path);
}


namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-getfilesize
 * @recoil-artifact defines .text recoil:function:0x4a5c50: zUtil::ZRDR_GetFileSize.
 * Purpose: return the file size for a resolved ZRDR path.
 */
int __fastcall ZRDR_GetFileSize(
    FILE *fileHandle
) {
    if (fileHandle == 0) {
        return 0;
    }

    const int originalOffset = ftell(fileHandle);
    fseek(
        fileHandle,
        0,
        SEEK_END
    );
    const int fileSize = ftell(fileHandle);
    fseek(
        fileHandle,
        originalOffset,
        SEEK_SET
    );
    return fileSize;
}
} // namespace zUtil


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-createsearchpathlist
 * @recoil-artifact defines .text recoil:function:0x4a5ca0: zUtil_ZRDR_CreateSearchPathList.
 * Purpose: allocate a search-path list and populate it from a path string.
 */
extern "C" zArchiveList *__fastcall zUtil_ZRDR_CreateSearchPathList(
    const char *pathText
) {
    zArchiveList *list = zArchiveList_CreateEmpty();
    zUtil::ZRDR_AddSearchPaths(
        list,
        pathText
    );
    return list;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-freesearchpathlist
 * @recoil-artifact defines .text recoil:function:0x4a5cc0: zUtil_ZRDR_FreeSearchPathList.
 * Purpose: free search-path payload strings and destroy the list container.
 */
extern "C" zArchiveList *__fastcall zUtil_ZRDR_FreeSearchPathList(
    zArchiveList *list
) {
    zUtil_ZRDR_FreePathList(list);
    zArchiveList_Destroy(list);
    return 0;
}


namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-addsearchpaths
 * @recoil-artifact defines .text recoil:function:0x4a5ce0: zUtil::ZRDR_AddSearchPaths.
 * Purpose: split and append semicolon-delimited paths to a search-path list.
 */
void __fastcall ZRDR_AddSearchPaths(
    zArchiveList *list,
    const char *pathText
) {
    if (pathText == 0) {
        return;
    }

    zArchiveList *activeList = list;
    while (true) {
        if (activeList != 0) {
            char *copy = _strdup(pathText);
            char *token = strtok(
                copy,
                g_zRdr_PathDelimStr
            );
            while (token != 0) {
                if (zReader_FileExists_Wrapper(token) != 0 &&
                    zArchiveList_FindPayloadByPredicate_Thunk(
                        activeList,
                        zUtil_ZRDR_StrCmpPredicate,
                        token
                    ) == 0) {
                    zArchiveList_PushFrontPayload(
                        activeList,
                        _strdup(token)
                    );
                }

                token = strtok(
                    0,
                    g_zRdr_PathDelimStr
                );
            }

            free(copy);
        }

        zArchiveList *scratchList = g_zRdr_ScratchSearchPathList;
        if (activeList == scratchList && activeList != 0) {
            return;
        }

        if (scratchList == 0) {
            scratchList = zArchiveList_CreateEmpty();
            g_zRdr_ScratchSearchPathList = scratchList;
        }

        activeList = scratchList;
    }
}
} // namespace zUtil


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-strcmppredicate
 * @recoil-artifact defines .text recoil:function:0x4a5da0: zUtil_ZRDR_StrCmpPredicate.
 * Purpose: compare a payload string against the requested string key.
 */
extern "C" int __fastcall zUtil_ZRDR_StrCmpPredicate(
    void *str1,
    void *str2
) {
    if (str1 == 0 || str2 == 0) {
        return 1;
    }

    return strcmp(
        (const char *)(str1),
        (const char *)(str2)
    );
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-freescratchsearchpathlist
 * @recoil-artifact defines .text recoil:function:0x4a5df0: zUtil_ZRDR_FreeScratchSearchPathList.
 * Purpose: release the scratch search-path list and clear its global pointer.
 */
extern "C" void __cdecl zUtil_ZRDR_FreeScratchSearchPathList() {
    if (g_zRdr_ScratchSearchPathList != 0) {
        zUtil_ZRDR_FreeSearchPathList(g_zRdr_ScratchSearchPathList);
    }

    g_zRdr_ScratchSearchPathList = 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-freepathlist
 * @recoil-artifact defines .text recoil:function:0x4a5e10: zUtil_ZRDR_FreePathList.
 * Purpose: free every search-path string payload from a search-path list.
 */
extern "C" int __fastcall zUtil_ZRDR_FreePathList(
    zArchiveList *list
) {
    zArchiveList *target = list;
    if (target == 0) {
        target = g_zRdr_ScratchSearchPathList;
    }

    void *payload = zArchiveList_PopFrontPayload(target);
    while (payload != 0) {
        free(payload);
        payload = zArchiveList_PopFrontPayload(target);
    }

    return 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-resolvepathinsearchpathlist
 * @recoil-artifact defines .text recoil:function:0x4a5e50: zUtil_ZRDR_ResolvePathInSearchPathList.
 * Purpose: resolve a filename through the supplied or scratch ZRDR search path.
 */
extern "C" char *__fastcall zUtil_ZRDR_ResolvePathInSearchPathList(
    zArchiveList *searchPathList,
    const char *filename
) {
    zArchiveList *list = searchPathList;
    _splitpath(
        filename,
        g_zRdr_SplitDriveBuf,
        g_zRdr_SplitDirBuf,
        g_zRdr_SplitFileNameBuf,
        g_zRdr_SplitExtBuf
    );
    sprintf(
        g_zRdr_ResolvedPathBuf,
        "%s%s",
        g_zRdr_SplitFileNameBuf,
        g_zRdr_SplitExtBuf
    );

    while (true) {
        if (list == 0 && g_zRdr_ScratchSearchPathList != 0) {
            list = g_zRdr_ScratchSearchPathList;
        }

        char *matchedDir = (char *)(zArchiveList_FindPayloadByPredicate(
            list,
            zUtil_ZRDR_SearchPathContainsFilePredicate,
            g_zRdr_ResolvedPathBuf
        ));
        if (matchedDir == 0) {
            zArchiveList *scratch = g_zRdr_ScratchSearchPathList;
            if (scratch == 0 || list == scratch) {
                return 0;
            }

            list = 0;
        } else {
            const size_t matchedDirLength = strlen(matchedDir);
            if (matchedDir[matchedDirLength - 1] == '\\') {
                matchedDir[matchedDirLength - 1] = '\0';
            }

            sprintf(
                g_zRdr_ResolvedPathBuf,
                g_zUtil_ZarPathJoinFmt,
                matchedDir,
                g_zRdr_SplitFileNameBuf,
                g_zRdr_SplitExtBuf
            );
            return g_zRdr_ResolvedPathBuf;
        }
    }
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-searchpathcontainsfilepredicate
 * @recoil-artifact defines .text recoil:function:0x4a5f20: zUtil_ZRDR_SearchPathContainsFilePredicate.
 * Purpose: join a search directory with a filename and report whether it exists.
 */
extern "C" int __fastcall zUtil_ZRDR_SearchPathContainsFilePredicate(
    void *searchDir,
    void *filename
) {
    sprintf(
        g_zRdr_PathJoinBuf,
        "%s\\%s",
        (const char *)(searchDir),
        (const char *)(filename)
    );
    return zReader::FileExists(g_zRdr_PathJoinBuf) == 0 ? 1 : 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-openfileresolved
 * @recoil-artifact defines .text recoil:function:0x4a5f50: zUtil_ZRDR_OpenFileResolved.
 * Purpose: open the resolved ZRDR search-path match, or fall back to the raw filename.
 */
extern "C" FILE *__fastcall zUtil_ZRDR_OpenFileResolved(
    zArchiveList *searchPathList,
    const char *filename,
    const char *mode
) {
    char *resolvedPath = zUtil_ZRDR_ResolvePathInSearchPathList(
        searchPathList,
        filename
    );
    if (resolvedPath != 0) {
        return fopen(resolvedPath, mode);
    }
    return fopen(filename, mode);
}


/**
 * Original-source helper evidence: no standalone retail function is present;
 * observed callers 0x4a5f90 and 0x4a6070 share the same wildcard digit rewrite
 * loop after initializing or incrementing the digit state.
 * Purpose: update wildcard digit placeholders in the active ZRDR wildcard path.
 */
#define ZUTIL_ZRDR_WRITE_WILDCARD_DIGITS()                                \
    do {                                                                  \
        for (int i = g_zUtil_ZRDR_WildcardStarCount - 1; i >= 0; --i) {  \
            char digitText[16];                                           \
            sprintf(                                                      \
                digitText,                                                \
                "%d",                                                    \
                g_zUtil_ZRDR_WildcardDigits[i]                            \
            );                                                            \
            *g_zUtil_ZRDR_WildcardStarPtrs[i] = digitText[0];             \
        }                                                                 \
    } while (0)


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-initwildcardpath
 * @recoil-artifact defines .text recoil:function:0x4a5f90: zUtil_ZRDR::InitWildcardPath.
 * Purpose: initialize wildcard path state from a path template.
 */
extern "C" char *__fastcall zUtil_ZRDR_InitWildcardPath(
    char *pattern
) {
    if (pattern == 0) {
        return 0;
    }

    for (int i = 0; i < 5; ++i) {
        g_zUtil_ZRDR_WildcardStarPtrs[i] = 0;
        g_zUtil_ZRDR_WildcardDigits[i] = 0;
    }

    g_zUtil_ZRDR_WildcardPath = pattern;
    g_zUtil_ZRDR_WildcardStarCount = 0;

    const int patternLength = (int)(strlen(pattern));
    for (int patternIndex = patternLength - 1; patternIndex >= 0; --patternIndex) {
        if (pattern[patternIndex] == '*') {
            g_zUtil_ZRDR_WildcardStarPtrs[g_zUtil_ZRDR_WildcardStarCount] = &pattern[patternIndex];
            ++g_zUtil_ZRDR_WildcardStarCount;
            if (g_zUtil_ZRDR_WildcardStarCount == 5) {
                break;
            }
        }
    }

    if (g_zUtil_ZRDR_WildcardStarCount == 0) {
        return 0;
    }

    ZUTIL_ZRDR_WRITE_WILDCARD_DIGITS();
    return g_zUtil_ZRDR_WildcardPath;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-nextwildcardpath
 * @recoil-artifact defines .text recoil:function:0x4a6070: zUtil_ZRDR::NextWildcardPath.
 * Purpose: advance wildcard digits and return the next generated path.
 */
extern "C" char *__cdecl zUtil_ZRDR_NextWildcardPath() {
    int carryOut = 0;
    int digitIndex = 0;
    if (g_zUtil_ZRDR_WildcardStarCount > 0) {
        while (digitIndex < g_zUtil_ZRDR_WildcardStarCount) {
            if (g_zUtil_ZRDR_WildcardDigits[digitIndex] < 9) {
                ++g_zUtil_ZRDR_WildcardDigits[digitIndex];
                carryOut = 0;
                break;
            }

            g_zUtil_ZRDR_WildcardDigits[digitIndex] = 0;
            ++digitIndex;
            carryOut = 1;
        }
    }

    if (carryOut != 0) {
        return 0;
    }

    ZUTIL_ZRDR_WRITE_WILDCARD_DIGITS();
    return g_zUtil_ZRDR_WildcardPath;
}

#undef ZUTIL_ZRDR_WRITE_WILDCARD_DIGITS


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zutil-zrdr-shutdownwildcardpath
 * @recoil-artifact defines .text recoil:function:0x4a6100: zUtil_ZRDR_ShutdownWildcardPath.
 * Purpose: free the active wildcard path buffer and reset wildcard state.
 */
extern "C" int __cdecl zUtil_ZRDR_ShutdownWildcardPath() {
    zUtil_ZRDR_FreeScratchSearchPathList();
    return 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zreader-readstring-gamezrecoil-zreader-zreader-cpp
 * @recoil-artifact defines .text recoil:function:0x4a6110: zReader_ReadString (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Reads a length-prefixed string payload, allocates a nul-terminated buffer, and returns bytes consumed.
 */
extern "C" int __fastcall zReader_ReadString(
    void *hFile,
    zReader::Value *outString
) {
    DWORD bytesRead;
    unsigned int length;
    ReadFile(
        (HANDLE)(hFile),
        &length,
        4,
        &bytesRead,
        0
    );
    int result = (int)(bytesRead);

    char *buffer = (char *)(malloc(length + 1));
    outString->str = buffer;
    memset(
        buffer,
        0,
        length + 1
    );

    if ((int)(length) > 0) {
        ReadFile(
            (HANDLE)(hFile),
            outString->str,
            length,
            &bytesRead,
            0
        );
        result += (int)(bytesRead);
    }

    return result;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-reset
 * @recoil-artifact defines .text recoil:function:0x4a6190: zIndexArchive::Reset.
 * Purpose: initialize archive fields to the closed empty state.
 */
zIndexArchive * zIndexArchive::Reset() {
    reservedFree = 0;
    hFile = INVALID_HANDLE_VALUE;
    recordCapacity = 0;
    recordCount = 0;
    records = 0;
    dirty = 0;
    return this;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-destroy
 * @recoil-artifact defines .text recoil:function:0x4a61b0: zIndexArchive::Destroy.
 * Purpose: close/free archive records and release the auxiliary reserved buffer.
 */
void zIndexArchive::Destroy() {
    CloseAndFreeRecords();
    if (reservedFree != 0) {
        free(reservedFree);
    }
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-init
 * @recoil-artifact defines .text recoil:function:0x4a61d0: zIndexArchive::Init.
 * Purpose: open an archive file for reading and load its trailing index.
 */
int zIndexArchive::Init(
    const char *filepath
) {
    HANDLE file = CreateFileA(
        filepath,
        GENERIC_READ,
        FILE_SHARE_READ,
        0,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS,
        0
    );

    int initialized = file != INVALID_HANDLE_VALUE;
    hFile = file;
    if (initialized != 0) {
        initialized = LoadIndexFromTail();
    } else {
        const DWORD lastError = GetLastError();
        char *message;
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            lastError,
            MAKELANGID(
                LANG_NEUTRAL,
                SUBLANG_DEFAULT
            ),
            (LPSTR)(&message),
            0,
            0
        );
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zUtil\\zutl_zar.cpp",
            0x4c,
            "GetLastError(0x%08x) : %s",
            lastError,
            message
        );
        LocalFree(message);
    }
    return initialized;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-opencreatewrite
 * @recoil-artifact defines .text recoil:function:0x4a6270: zIndexArchive::OpenCreateWrite.
 * Purpose: create an archive file for writing and initialize record storage.
 */
int zIndexArchive::OpenCreateWrite(
    const char *filepath
) {
    HANDLE file = CreateFileA(
        filepath,
        GENERIC_READ | GENERIC_WRITE,
        0,
        0,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        0
    );

    hFile = file;
    return file != INVALID_HANDLE_VALUE ? 1 : 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-closeandfreerecords
 * @recoil-artifact defines .text recoil:function:0x4a62b0: zIndexArchive::CloseAndFreeRecords.
 * Purpose: flush dirty records, close the file handle, and reset record storage.
 */
int zIndexArchive::CloseAndFreeRecords() {
    if (dirty != 0) {
        FlushIndexToTail();
    }

    HANDLE const file = (HANDLE)(hFile);
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }

    hFile = INVALID_HANDLE_VALUE;
    FreeRecordsAndReset();
    return 1;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-ensurecapacity
 * @recoil-artifact defines .text recoil:function:0x4a62f0: zIndexArchive::EnsureCapacity.
 * Purpose: grow the record table capacity to hold the requested record count.
 */
void zIndexArchive::EnsureCapacity(
    unsigned int requiredCount
) {
    if (requiredCount < recordCapacity) {
        return;
    }

    unsigned int newCapacity = recordCapacity * 2;
    const unsigned int requiredPlusOne = requiredCount + 1;
    if (newCapacity <= requiredPlusOne) {
        newCapacity = requiredPlusOne;
    }

    recordCapacity = newCapacity;
    records = (zZarFileRecord *)(realloc(
        records,
        (size_t)(newCapacity) * sizeof(zZarFileRecord)
    ));
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-freerecordsandreset
 * @recoil-artifact defines .text recoil:function:0x4a6330: zIndexArchive::FreeRecordsAndReset.
 * Purpose: free the archive record table and restore the closed empty fields.
 */
void zIndexArchive::FreeRecordsAndReset() {
    if (records == 0) {
        return;
    }

    free(records);
    reservedFree = 0;
    hFile = INVALID_HANDLE_VALUE;
    recordCapacity = 0;
    recordCount = 0;
    records = 0;
    dirty = 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-flushindextotail
 * @recoil-artifact defines .text recoil:function:0x4a6360: zIndexArchive::FlushIndexToTail.
 * Purpose: append the record table and tail metadata to the archive file.
 */
void zIndexArchive::FlushIndexToTail() {
    DWORD numberOfBytesWritten = 0;
    const unsigned int footerMagic = 1;
    const unsigned int recordBytes = recordCount * sizeof(zZarFileRecord);
    unsigned int *const recordCountFooter = &recordCount;
    SetFilePointer(
        (HANDLE)(hFile),
        0,
        0,
        FILE_END
    );
    WriteFile(
        (HANDLE)(hFile),
        records,
        recordBytes,
        &numberOfBytesWritten,
        0
    );
    WriteFile(
        (HANDLE)(hFile),
        &footerMagic,
        sizeof(footerMagic),
        &numberOfBytesWritten,
        0
    );
    WriteFile(
        (HANDLE)(hFile),
        recordCountFooter,
        sizeof(*recordCountFooter),
        &numberOfBytesWritten,
        0
    );
    dirty = 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-loadindexfromtail
 * @recoil-artifact defines .text recoil:function:0x4a63f0: zIndexArchive::LoadIndexFromTail.
 * Purpose: read and validate the archive index footer and record table.
 */
int zIndexArchive::LoadIndexFromTail() {
    if (GetFileSize(
        (HANDLE)(hFile),
        0
    ) < 8) {
        return 0;
    }

    SetFilePointer(
        (HANDLE)(hFile),
        -8,
        0,
        FILE_END
    );

    DWORD numberOfBytesRead;
    unsigned int footerMagic;
    unsigned int recordCountFromTail;
    ReadFile(
        (HANDLE)(hFile),
        &footerMagic,
        sizeof(footerMagic),
        &numberOfBytesRead,
        0
    );
    ReadFile(
        (HANDLE)(hFile),
        &recordCountFromTail,
        sizeof(recordCountFromTail),
        &numberOfBytesRead,
        0
    );

    if (footerMagic != 1) {
        return 0;
    }

    EnsureCapacity(recordCountFromTail);
    const unsigned int bytesToRead = recordCountFromTail * sizeof(zZarFileRecord);
    SetFilePointer(
        (HANDLE)(hFile),
        -8 - (LONG)(bytesToRead),
        0,
        FILE_END
    );
    ReadFile(
        (HANDLE)(hFile),
        records,
        bytesToRead,
        &numberOfBytesRead,
        0
    );
    SetFilePointer(
        (HANDLE)(hFile),
        0,
        0,
        FILE_BEGIN
    );
    recordCount = recordCountFromTail;
    return 1;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-addfilerecord
 * @recoil-artifact defines .text recoil:function:0x4a64d0: zIndexArchive::AddFileRecord.
 * Purpose: append a named payload to the archive file and record its index data.
 */
int zIndexArchive::AddFileRecord(
    const char *name,
    const void *data,
    unsigned int dataSize,
    const char *sourceTempPathOrNull,
    const zZarFileTime *sourceFileTimeOrNull
) {
    const unsigned int oldRecordCount = recordCount;
    EnsureCapacity(oldRecordCount + 1);

    SetFilePointer(
        (HANDLE)(hFile),
        0,
        0,
        FILE_END
    );

    zZarFileRecord record;
    record.fileOffset = GetFileSize(
        (HANDLE)(hFile),
        0
    );
    record.fileSize = dataSize;

    if (sourceTempPathOrNull != 0) {
        record.recordFlags |= 2;
        strncpy(
            record.sourceTempPath,
            sourceTempPathOrNull,
            sizeof(record.sourceTempPath)
        );
        if (sourceFileTimeOrNull != 0) {
            record.sourceFileTimeLow = sourceFileTimeOrNull->lowDateTime;
            record.sourceFileTimeHigh = sourceFileTimeOrNull->highDateTime;
        }
    }

    strncpy(
        record.name,
        name,
        sizeof(record.name)
    );

    DWORD numberOfBytesWritten = 0;
    WriteFile(
        (HANDLE)(hFile),
        data,
        dataSize,
        &numberOfBytesWritten,
        0
    );

    records[oldRecordCount] = record;
    ++recordCount;
    dirty = 1;
    return numberOfBytesWritten == dataSize ? 1 : 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-findrecordbynameci
 * @recoil-artifact defines .text recoil:function:0x4a65d0: zIndexArchive::FindRecordByNameCI.
 * Purpose: find an archive file record by case-insensitive name.
 */
zZarFileRecord * zIndexArchive::FindRecordByNameCI(
    const char *filename
) {
    for (unsigned int i = 0; i < recordCount; ++i) {
        zZarFileRecord *record = &records[i];
        if (_stricmp(
            filename,
            record->name
        ) == 0) {
            return &records[i];
        }
    }

    return 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-openfilebyname
 * @recoil-artifact defines .text recoil:function:0x4a6630: zIndexArchive::OpenFileByName.
 * Purpose: open a file member from the archive and optionally return its size.
 */
void * zIndexArchive::OpenFileByName(
    const char *filename,
    unsigned int *outSize
) {
    zZarFileRecord *record = FindRecordByNameCI(filename);
    if (record == 0) {
        return INVALID_HANDLE_VALUE;
    }

    if (outSize != 0) {
        *outSize = record->fileSize;
    }

    SetFilePointer(
        (HANDLE)(hFile),
        record->fileOffset,
        0,
        FILE_BEGIN
    );
    return hFile;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zar-zindexarchive-readfilebyname
 * @recoil-artifact defines .text recoil:function:0x4a6670: zIndexArchive::ReadFileByName.
 * Purpose: read a named archive member into the caller-provided buffer.
 */
int zIndexArchive::ReadFileByName(
    const char *filename,
    void *buffer,
    unsigned int *bufferSize
) {
    zZarFileRecord *const record = FindRecordByNameCI(filename);
    if (record == 0) {
        return 0x10001;
    }

    const unsigned int availableBytes = *bufferSize;
    *bufferSize = record->fileSize;
    if (*bufferSize > availableBytes) {
        return 0x10002;
    }

    SetFilePointer(
        (HANDLE)(hFile),
        record->fileOffset,
        0,
        FILE_BEGIN
    );
    DWORD bytesRead;
    ReadFile(
        (HANDLE)(hFile),
        buffer,
        record->fileSize,
        &bytesRead,
        0
    );
    return 0;
}
