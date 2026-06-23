#include "zReader.h"

#include "Battlesport/HudSensorTracker.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zError/zError.h"
#include "zClass.h"

#include <windows.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reimplements data 0x56ae70: g_zUtil_ZRDR_FreePool.
 * Purpose: store reusable archive-list nodes for ZRDR list operations.
 */
extern "C" zArchiveList *g_zUtil_ZRDR_FreePool = 0;
/**
 * Reimplements data 0x56ae74: g_zUtil_ZRDR_TotalAllocated.
 * Purpose: count archive-list nodes allocated for the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_TotalAllocated = 0;
/**
 * Reimplements data 0x56ae78: g_zUtil_ZRDR_FreeCount.
 * Purpose: count nodes currently held by the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_FreeCount = 0;
/**
 * Reimplements data 0x56ae7c: g_zUtil_ZRDR_GrowCount.
 * Purpose: count demand-growth events for the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_GrowCount = 0;
/**
 * Reimplements data 0x56ae80: g_zReader_FileExtBuf.
 * Purpose: store the file extension split out before archive-member lookup.
 */
extern "C" char g_zReader_FileExtBuf[0x100] = {0};
/**
 * Reimplements data 0x56af80: g_zReader_FileNameBuf.
 * Purpose: store the basename joined with extension for archive-member lookup.
 */
extern "C" char g_zReader_FileNameBuf[0x100] = {0};
/**
 * Reimplements data 0x56b180: g_zRdr_SearchPathList.
 * Purpose: store the global ZRDR search-path list.
 */
extern "C" zArchiveList *g_zRdr_SearchPathList = 0;
/**
 * Reimplements data 0x56b184: g_zArchive_MountedList.
 * Purpose: store the process-wide list of mounted ZRDR/ZAR index archives.
 */
extern "C" zArchiveList *g_zArchive_MountedList = 0;
/**
 * Reimplements data 0x56b188: g_zArchive_Current.
 * Purpose: store the current mounted archive selected by archive mounting.
 */
extern "C" zIndexArchive *g_zArchive_Current = 0;
/**
 * Reimplements data 0x56b678: g_zRdr_SplitFileNameBuf.
 * Purpose: store the CRT basename component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitFileNameBuf[0x100] = {0};
/**
 * Reimplements data 0x56b778: g_zRdr_SplitExtBuf.
 * Purpose: store the CRT extension component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitExtBuf[0x100] = {0};
/**
 * Reimplements data 0x56b878: g_zRdr_PathJoinBuf.
 * Purpose: store joined search-directory and filename probes.
 */
extern "C" char g_zRdr_PathJoinBuf[0x100] = {0};
/**
 * Reimplements data 0x56b980: g_zRdr_ResolvedPathBuf.
 * Purpose: store the resolved path returned from ZRDR search-path lookup.
 */
extern "C" char g_zRdr_ResolvedPathBuf[0x100] = {0};
/**
 * Reimplements data 0x56ba88: g_zRdr_SplitDirBuf.
 * Purpose: store the CRT directory component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitDirBuf[0x100] = {0};
/**
 * Reimplements data 0x56bb88: g_zRdr_SplitDriveBuf.
 * Purpose: store the CRT drive component during ZRDR path resolution.
 */
extern "C" char g_zRdr_SplitDriveBuf[4] = {0};
/**
 * Reimplements data 0x56bb8c: g_zRdr_ScratchSearchPathList.
 * Purpose: store the temporary ZRDR search-path list used by scoped lookups.
 */
extern "C" zArchiveList *g_zRdr_ScratchSearchPathList = 0;
/**
 * Reimplements data 0x56bb90: g_zUtil_ZRDR_WildcardDigits.
 * Purpose: store the odometer digits for active wildcard path expansion.
 */
extern "C" int g_zUtil_ZRDR_WildcardDigits[5] = {0};
/**
 * Reimplements data 0x56bba4: g_zUtil_ZRDR_WildcardPath.
 * Purpose: store the active in-place wildcard path string.
 */
extern "C" char *g_zUtil_ZRDR_WildcardPath = 0;
/**
 * Reimplements data 0x56bba8: g_zUtil_ZRDR_WildcardStarCount.
 * Purpose: store the number of active wildcard placeholders, capped at five.
 */
extern "C" int g_zUtil_ZRDR_WildcardStarCount = 0;
/**
 * Reimplements reserved data slot 0x56bbac in the ZRDR wildcard state.
 * Purpose: preserve the unreferenced zero-initialized dword between the
 * wildcard count and pointer array proven by BN.
 */
extern "C" int g_zUtil_ZRDR_WildcardReserved = 0;
/**
 * Reimplements data 0x56bbb0: g_zUtil_ZRDR_WildcardStarPtrs.
 * Purpose: store pointers to wildcard placeholder bytes inside the active path.
 */
extern "C" char *g_zUtil_ZRDR_WildcardStarPtrs[5] = {0};
/**
 * Reimplements data 0x4f3ab4: g_Mover_LastLoadedNode.
 * Purpose: remember the most recent mover node accepted from movers.zrd.
 */
extern "C" zClass_NodePartial *g_Mover_LastLoadedNode = 0;

/**
 * Reimplements data 0x4dcbe8: g_zUtil_MissionZrdrArchivePathFmt.
 * Purpose: format the mission-specific ZRDR archive path mounted after search-path setup.
 */
const char g_zUtil_MissionZrdrArchivePathFmt[0x11] = "zbd\\m%d\\zrdr.zbd";
/**
 * Reimplements data 0x4dcbfc: g_zUtil_MissionZrdrSearchPathsFmt.
 * Purpose: format the loose mission ZRDR search paths before mounting the archive.
 */
const char g_zUtil_MissionZrdrSearchPathsFmt[0x3d] =
    "..\\data\\common\\zrdr;..\\data\\m%d\\zrdr;..\\data\\m%d\\zrdr\\aipath";
/**
 * Reimplements data 0x4dcc3c: g_zImage_CommonTextureSearchPaths.
 * Purpose: supplies common texture and effect texture search paths for mission resources.
 */
const char g_zImage_CommonTextureSearchPaths[0x38] =
    "..\\data\\common\\textures;..\\data\\common\\effects\\textures";
/**
 * Reimplements data 0x4e3008: g_zUtil_ZarPathJoinFmt.
 * Purpose: format a matched ZRDR search directory with the split filename and extension.
 */
extern "C" char g_zUtil_ZarPathJoinFmt[0x8] = "%s\\%s%s";

/**
 * Reimplements 0x48c9a0: zArchiveList_LinkNodeBetween.
 * Purpose: link a node between two existing circular-list neighbors.
 */
extern "C" void __fastcall zArchiveList_LinkNodeBetween(
    zArchiveListNode *after,
    zArchiveListNode *newNode,
    zArchiveListNode *before
) {
    after->next = newNode;
    before->prev = newNode;
    newNode->next = before;
    newNode->prev = after;
}

/**
 * Reimplements 0x48c950: zArchiveList_CreateEmpty.
 * Purpose: allocate and initialize an empty circular archive list.
 */
extern "C" zArchiveList *zArchiveList_CreateEmpty() {
    zArchiveList *result = (zArchiveList *)(malloc(sizeof(zArchiveList)));
    memset(
        result,
        0,
        sizeof(zArchiveList)
    );
    return result;
}

/**
 * Reimplements 0x48c970: zArchiveList_Destroy.
 * Purpose: drain an archive-list container and release the list allocation.
 */
extern "C" int __fastcall zArchiveList_Destroy(
    zArchiveList *list
) {
    if (list != 0) {
        while (zArchiveList_PopFrontPayload(list) != 0) {
        }

        free(list);
    }

    return 0;
}

/**
 * Reimplements 0x48c820: zUtil_ZRDR_PushFreeNode.
 * Purpose: return a node to the shared ZRDR free-node list.
 */
extern "C" void __fastcall zUtil_ZRDR_PushFreeNode(
    zArchiveListNode *node
) {
    if (g_zUtil_ZRDR_FreePool == 0) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    zArchiveListNode *head = g_zUtil_ZRDR_FreePool->head;
    if (head == 0) {
        g_zUtil_ZRDR_FreePool->head = node;
        node->next = node;
        node->prev = node;
    } else {
        zArchiveList_LinkNodeBetween(
            head->prev,
            node,
            head
        );
        g_zUtil_ZRDR_FreePool->head = node;
    }

    ++g_zUtil_ZRDR_FreePool->count;
    ++g_zUtil_ZRDR_FreeCount;
}

/**
 * Reimplements 0x48c800: zUtil_ZRDR_GrowFreePool.
 * Purpose: allocate a batch of reusable ZRDR archive-list nodes.
 */
extern "C" void zUtil_ZRDR_GrowFreePool() {
    zArchiveListNode *node = (zArchiveListNode *)(malloc(sizeof(zArchiveListNode)));
    zUtil_ZRDR_PushFreeNode(node);
    ++g_zUtil_ZRDR_TotalAllocated;
}

namespace zUtil {
/**
 * Reimplements 0x48c7d0: zUtil::ZRDR_PreallocNodePool.
 * Purpose: ensure the free-node pool has at least the requested node count.
 */
void __fastcall ZRDR_PreallocNodePool(
    int count
) {
    if (g_zUtil_ZRDR_FreePool != 0) {
        return;
    }

    g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    while (count > 0) {
        zUtil_ZRDR_GrowFreePool();
        --count;
    }
}
} // namespace zUtil

/**
 * Reimplements 0x48c8e0: zUtil_ZRDR_PopFreeNode.
 * Purpose: pop one reusable node from the ZRDR free-node list.
 */
extern "C" zArchiveListNode *__fastcall zUtil_ZRDR_PopFreeNode(
    int allowGrow
) {
    zArchiveList *pool = g_zUtil_ZRDR_FreePool;
    if (pool->count == 0) {
        if (allowGrow == 0) {
            return 0;
        }

        zUtil_ZRDR_GrowFreePool();
        ++g_zUtil_ZRDR_GrowCount;
    }

    const int count = pool->count;
    zArchiveListNode *head = pool->head;
    if (count == 1) {
        pool->head = 0;
        pool->count = count - 1;
        --g_zUtil_ZRDR_FreeCount;
        return head;
    }

    head->prev->next = head->next;
    head->next->prev = head->prev;
    pool->head = head->next;
    pool->count = count - 1;
    --g_zUtil_ZRDR_FreeCount;
    return head;
}

/**
 * Reimplements 0x48ca10: zUtil_ZRDR_AllocNodeWithPayload.
 * Purpose: allocate a list node and attach the supplied payload pointer.
 */
extern "C" zArchiveListNode *__fastcall zUtil_ZRDR_AllocNodeWithPayload(
    void *payload
) {
    zArchiveListNode *result = zUtil_ZRDR_PopFreeNode(1);
    result->payload = payload;
    return result;
}

/**
 * Reimplements 0x48c9c0: zArchiveList_PushFrontPayload.
 * Purpose: insert a payload node at the head of an archive list.
 */
extern "C" int __fastcall zArchiveList_PushFrontPayload(
    zArchiveList *list,
    void *payload
) {
    if (list == 0) {
        return -1;
    }

    zArchiveListNode *newNode = zUtil_ZRDR_AllocNodeWithPayload(payload);
    zArchiveListNode *head = list->head;
    if (head == 0) {
        list->head = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        zArchiveList_LinkNodeBetween(
            head->prev,
            newNode,
            head
        );
        list->head = newNode;
    }

    ++list->count;
    return list->count;
}

/**
 * Reimplements 0x48ca30: zArchiveList_PushBackPayload.
 * Purpose: insert a payload node at the tail of an archive list.
 */
extern "C" int __fastcall zArchiveList_PushBackPayload(
    zArchiveList *list,
    void *payload
) {
    if (list == 0) {
        return -1;
    }

    zArchiveListNode *newNode = zUtil_ZRDR_AllocNodeWithPayload(payload);
    zArchiveListNode *head = list->head;
    if (head == 0) {
        list->head = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        zArchiveList_LinkNodeBetween(
            head->prev,
            newNode,
            head
        );
    }

    ++list->count;
    return list->count;
}

/**
 * Reimplements 0x48ca70: zArchiveList_RemovePayload.
 * Purpose: remove a matching payload node from a circular archive list and
 * return the remaining node count.
 */
extern "C" int __fastcall zArchiveList_RemovePayload(
    zArchiveList *list,
    void *payload
) {
    if (list == 0 || list->count == 0) {
        return -1;
    }

    zArchiveListNode *const node = zArchiveList_FindNodeByPayload(
        list,
        payload
    );
    if (node == 0) {
        return -1;
    }

    if (list->count == 1) {
        zArchiveList_FreeNode(node);
        list->head = 0;
        --list->count;
        return list->count;
    }

    if (list->head == node) {
        list->head = node->next;
    }

    node->prev->next = node->next;
    node->next->prev = node->prev;
    zArchiveList_FreeNode(node);
    --list->count;
    return list->count;
}

/**
 * Reimplements 0x48cae0: zArchiveList_FreeNode.
 * Purpose: return one archive-list node to the shared node pool and return its
 * payload pointer.
 */
extern "C" void *__fastcall zArchiveList_FreeNode(
    zArchiveListNode *node
) {
    if (node == 0) {
        return 0;
    }

    void *payload = node->payload;
    zUtil_ZRDR_PushFreeNode(node);
    return payload;
}

/**
 * Reimplements 0x48cb00: zArchiveList_FindNodeByPayload.
 * Purpose: find the circular-list node that owns a specific payload pointer.
 */
extern "C" zArchiveListNode *__fastcall zArchiveList_FindNodeByPayload(
    zArchiveList *list,
    void *payload
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *const head = list->head;
    zArchiveListNode *node = head;
    if (node->payload == payload) {
        return node;
    }

    while (true) {
        node = node->next;
        if (node == head) {
            return 0;
        }

        if (node->payload == payload) {
            return node;
        }
    }
}

/**
 * Reimplements 0x48cb70: zArchiveList_PopFrontPayload.
 * Purpose: unlink the head node from an archive list and return its payload.
 */
extern "C" void *__fastcall zArchiveList_PopFrontPayload(
    zArchiveList *list
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *head = list->head;
    if (list->count == 1) {
        list->head = 0;
        --list->count;
        return zArchiveList_FreeNode(head);
    }

    head->prev->next = head->next;
    head->next->prev = head->prev;
    list->head = head->next;
    --list->count;
    return zArchiveList_FreeNode(head);
}

/**
 * Reimplements 0x48cc60: zArchiveList_GetCount.
 * Purpose: return the number of payload nodes in an archive list.
 */
extern "C" int __fastcall zArchiveList_GetCount(
    zArchiveList *list
) {
    if (list == 0) {
        return 0;
    }

    return list->count;
}

/**
 * Reimplements 0x48cb30: zArchiveList_GetAt.
 * Purpose: return the payload at a zero-based archive-list index.
 */
extern "C" void *__fastcall zArchiveList_GetAt(
    zArchiveList *list,
    int index
) {
    if (list == 0 || index >= list->count) {
        return 0;
    }

    zArchiveListNode *head = list->head;
    if (index == 0) {
        return head->payload;
    }

    int i = 1;
    zArchiveListNode *node = head->next;
    while (node != head && i != index) {
        node = node->next;
        ++i;
    }

    return node->payload;
}

/**
 * Reimplements 0x48cbd0: zArchiveList_FindPayloadByPredicate.
 * Purpose: find the first payload accepted by a caller-supplied predicate.
 */
extern "C" void *__fastcall zArchiveList_FindPayloadByPredicate(
    zArchiveList *list,
    zArchiveListPredicate predicate,
    void *userData
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    if (predicate(
        node->payload,
        userData
    ) == 0) {
        goto found;
    }

    while (true) {
        node = node->next;
        if (node == list->head) {
            return 0;
        }

        if (predicate(
            node->payload,
            userData
        ) == 0) {
            goto found;
        }
    }

found:
    return node->payload;
}

/**
 * Reimplements 0x48cc20: zArchiveList_FindPayloadByValue.
 * Purpose: find the first payload whose leading dword equals the requested
 * value.
 */
extern "C" void *__fastcall zArchiveList_FindPayloadByValue(
    zArchiveList *list,
    unsigned int value
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *const head = list->head;
    zArchiveListNode *node = head;
    if (*(unsigned int *)(node->payload) == value) {
        return node->payload;
    }

    while (true) {
        node = node->next;
        if (node == head) {
            return 0;
        }

        if (*(unsigned int *)(node->payload) == value) {
            return node->payload;
        }
    }
}

/**
 * Reimplements 0x48cc50: zArchiveList_FindPayloadByPredicate_Thunk.
 * Purpose: adapt predicate payload search to the original fastcall callback shape.
 */
extern "C" void *__fastcall zArchiveList_FindPayloadByPredicate_Thunk(
    zArchiveList *list,
    zArchiveListPredicate predicate,
    void *userData
) {
    return zArchiveList_FindPayloadByPredicate(
        list,
        predicate,
        userData
    );
}

/**
 * Reimplements 0x4a5da0: zUtil_ZRDR_StrCmpPredicate.
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
 * Reimplements 0x4a5f20: zUtil_ZRDR_SearchPathContainsFilePredicate.
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

namespace zUtil {
/**
 * Reimplements 0x4a5c50: zUtil::ZRDR_GetFileSize.
 * Source: D:\Proj\GameZRecoil\zUtil\zutl_zrdr.cpp.
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
 * Reimplements 0x4a5e50: zUtil_ZRDR_ResolvePathInSearchPathList.
 * Purpose: resolve a filename through the supplied or scratch ZRDR search path.
 */
extern "C" char *__fastcall zUtil_ZRDR_ResolvePathInSearchPathList(
    zArchiveList *searchPathList,
    const char *filename
) {
    zArchiveList *list = searchPathList;
    while (true) {
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
            if (matchedDir[strlen(matchedDir) - 1] == '\\') {
                matchedDir[strlen(matchedDir) - 1] = '\0';
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
 * Reimplements 0x4a5f50: zUtil_ZRDR_OpenFileResolved.
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
    return fopen(
        resolvedPath != 0 ? resolvedPath : filename,
        mode
    );
}

namespace {
/**
 * Original-source helper evidence: no standalone retail function is present;
 * observed callers 0x4a5f90 and 0x4a6070 share the same wildcard digit rewrite
 * loop after initializing or incrementing the digit state.
 * Purpose: update wildcard digit placeholders in the active ZRDR wildcard path.
 */
void zUtil_ZRDR_WriteWildcardDigits() {
    for (int i = g_zUtil_ZRDR_WildcardStarCount - 1; i >= 0; --i) {
        char digitText[16];
        sprintf(
            digitText,
            "%d",
            g_zUtil_ZRDR_WildcardDigits[i]
        );
        *g_zUtil_ZRDR_WildcardStarPtrs[i] = digitText[0];
    }
}
} // namespace

/**
 * Reimplements 0x4a5f90: zUtil_ZRDR::InitWildcardPath.
 * Source: D:\Proj\GameZRecoil\zUtil\zutl_zrdr.cpp.
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

    zUtil_ZRDR_WriteWildcardDigits();
    return g_zUtil_ZRDR_WildcardPath;
}

/**
 * Reimplements 0x4a6070: zUtil_ZRDR::NextWildcardPath.
 * Source: D:\Proj\GameZRecoil\zUtil\zutl_zrdr.cpp.
 * Purpose: advance wildcard digits and return the next generated path.
 */
extern "C" char * zUtil_ZRDR_NextWildcardPath() {
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

    zUtil_ZRDR_WriteWildcardDigits();
    return g_zUtil_ZRDR_WildcardPath;
}

namespace zUtil {
/**
 * Reimplements 0x4a5ce0: zUtil::ZRDR_AddSearchPaths.
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
                ";"
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
                    ";"
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
 * Reimplements 0x4a5ca0: zUtil_ZRDR_CreateSearchPathList.
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
 * Reimplements 0x4a5e10: zUtil_ZRDR_FreePathList.
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
 * Reimplements 0x4a5cc0: zUtil_ZRDR_FreeSearchPathList.
 * Purpose: free search-path payload strings and destroy the list container.
 */
extern "C" zArchiveList *__fastcall zUtil_ZRDR_FreeSearchPathList(
    zArchiveList *list
) {
    zUtil_ZRDR_FreePathList(list);
    zArchiveList_Destroy(list);
    return 0;
}

/**
 * Reimplements 0x4a5df0: zUtil_ZRDR_FreeScratchSearchPathList.
 * Purpose: release the scratch search-path list and clear its global pointer.
 */
extern "C" void zUtil_ZRDR_FreeScratchSearchPathList() {
    if (g_zRdr_ScratchSearchPathList != 0) {
        zUtil_ZRDR_FreeSearchPathList(g_zRdr_ScratchSearchPathList);
    }

    g_zRdr_ScratchSearchPathList = 0;
}

/**
 * Reimplements 0x4a6100: zUtil_ZRDR_ShutdownWildcardPath.
 * Purpose: free the active wildcard path buffer and reset wildcard state.
 */
extern "C" int zUtil_ZRDR_ShutdownWildcardPath() {
    zUtil_ZRDR_FreeScratchSearchPathList();
    return 0;
}

/**
 * Reimplements 0x48d2c0: zUtil_ZRDR_UnloadMountedArchives.
 * Purpose: destroy mounted archives while optionally preserving the current archive.
 */
extern "C" int __fastcall zUtil_ZRDR_UnloadMountedArchives(
    int destroyCurrentToo
) {
    zIndexArchive *archive =
        (zIndexArchive *)(zArchiveList_PopFrontPayload(g_zArchive_MountedList));
    while (archive != 0) {
        zIndexArchive *const current = g_zArchive_Current;
        if (destroyCurrentToo != 0 || archive != current) {
            if (current == archive) {
                g_zArchive_Current = 0;
            }

            archive->CloseAndFreeRecords();
            archive->Destroy();
            ::operator delete(archive);
        }

        archive = (zIndexArchive *)(zArchiveList_PopFrontPayload(g_zArchive_MountedList));
    }

    if (destroyCurrentToo == 0 && g_zArchive_Current != 0) {
        return zArchiveList_PushBackPayload(
            g_zArchive_MountedList,
            g_zArchive_Current
        );
    }

    return 0;
}

/**
 * Reimplements 0x48cd10: zUtil_ZRDR_Shutdown.
 * Purpose: shut down ZRDR path state, mounted archives, and node pools.
 */
extern "C" int zUtil_ZRDR_Shutdown() {
    zUtil_ZRDR_FreeSearchPathList(g_zRdr_SearchPathList);
    zUtil_ZRDR_UnloadMountedArchives(1);
    zArchiveList_Destroy(g_zArchive_MountedList);
    g_zRdr_SearchPathList = 0;
    g_zArchive_MountedList = 0;
    return 0;
}

/**
 * Reimplements 0x48c890: zUtil_ZRDR_FreeNodePool.
 * Purpose: release all nodes currently held in the ZRDR free-node pool.
 */
extern "C" void zUtil_ZRDR_FreeNodePool() {
    if (g_zUtil_ZRDR_FreePool == 0) {
        return;
    }

    zArchiveListNode *node = zUtil_ZRDR_PopFreeNode(0);
    while (node != 0) {
        free(node);
        node = zUtil_ZRDR_PopFreeNode(0);
    }

    zArchiveList_Destroy(g_zUtil_ZRDR_FreePool);
    g_zUtil_ZRDR_FreePool = 0;
}

/**
 * Reimplements 0x48cca0: zUtil_ZRDR_SetSearchPath.
 * Purpose: replace the current ZRDR search path list.
 */
extern "C" int __fastcall zUtil_ZRDR_SetSearchPath(
    const char *pathText
) {
    if (g_zRdr_SearchPathList == 0) {
        g_zRdr_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil_ZRDR_FreePathList(g_zRdr_SearchPathList);
    zUtil::ZRDR_AddSearchPaths(
        g_zRdr_SearchPathList,
        pathText
    );
    return 0;
}

/**
 * Reimplements 0x48cce0: zUtil_ZRDR_AppendSearchPath.
 * Purpose: append additional paths to the current ZRDR search path list.
 */
extern "C" int __fastcall zUtil_ZRDR_AppendSearchPath(
    const char *pathText
) {
    if (g_zRdr_SearchPathList == 0) {
        g_zRdr_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil::ZRDR_AddSearchPaths(
        g_zRdr_SearchPathList,
        pathText
    );
    return 0;
}

namespace zUtil {
/**
 * Reimplements 0x48cc70: zUtil::ZRDR_Init.
 * Purpose: initialize ZRDR search-path and node-pool state.
 */
int __fastcall ZRDR_Init(
    const char *pathText
) {
    if (g_zArchive_MountedList == 0) {
        g_zArchive_MountedList = zArchiveList_CreateEmpty();
        zUtil_ZRDR_SetSearchPath(pathText);
        g_zArchive_Current = 0;
    }

    return 0;
}

/**
 * Reimplements 0x42ecb0: zUtil::SetMissionZrdrPathsAndMountZbd.
 * Source: D:\Proj\GameZRecoil\zUtil\zUtil.cpp.
 * Purpose: configure mission ZRDR search paths and mount the mission archive.
 */
int __fastcall SetMissionZrdrPathsAndMountZbd(
    int missionId
) {
    char pathText[256];

    zUtil_ZRDR_FreePathList(0);
    ZRDR_AddSearchPaths(
        0,
        "zbd"
    );
    zImage_InitMissionResources(g_zImage_CommonTextureSearchPaths);

    sprintf(
        pathText,
        g_zUtil_MissionZrdrSearchPathsFmt,
        missionId,
        missionId
    );
    zUtil_ZRDR_SetSearchPath(pathText);

    if (g_HudSensorTracker.missionFlags == 0) {
        return 0;
    }

    sprintf(
        pathText,
        g_zUtil_MissionZrdrArchivePathFmt,
        missionId
    );
    return zArchive::MountIndexArchive(
        pathText,
        0
    );
}
} // namespace zUtil

/**
 * Reimplements 0x4a6190: zIndexArchive::Reset.
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
 * Reimplements 0x4a61b0: zIndexArchive::Destroy.
 * Purpose: close/free archive records and release the auxiliary reserved buffer.
 */
void zIndexArchive::Destroy() {
    CloseAndFreeRecords();
    if (reservedFree != 0) {
        free(reservedFree);
    }
}

/**
 * Reimplements 0x4a61d0: zIndexArchive::Init.
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
 * Reimplements 0x4a6270: zIndexArchive::OpenCreateWrite.
 * Source: D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp.
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
 * Reimplements 0x4a62b0: zIndexArchive::CloseAndFreeRecords.
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
 * Reimplements 0x4a6330: zIndexArchive::FreeRecordsAndReset.
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
 * Reimplements 0x4a6360: zIndexArchive::FlushIndexToTail.
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
 * Reimplements 0x4a63f0: zIndexArchive::LoadIndexFromTail.
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
 * Reimplements 0x4a62f0: zIndexArchive::EnsureCapacity.
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
 * Reimplements 0x4a64d0: zIndexArchive::AddFileRecord.
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

    HANDLE const file = (HANDLE)(hFile);
    SetFilePointer(
        file,
        0,
        0,
        FILE_END
    );

    zZarFileRecord record;
    record.fileOffset = GetFileSize(
        file,
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
        file,
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
 * Reimplements 0x4a65d0: zIndexArchive::FindRecordByNameCI.
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
 * Reimplements 0x4a6630: zIndexArchive::OpenFileByName.
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
 * Reimplements 0x4a6670: zIndexArchive::ReadFileByName.
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

namespace zArchive {
/**
 * Reimplements 0x48d210: zArchive::MountIndexArchive.
 * Purpose: allocate, initialize, and register a mounted index archive.
 */
int __fastcall MountIndexArchive(
    const char *path,
    int setCurrent
) {
    zIndexArchive *archive = new zIndexArchive;
    zIndexArchive *payload = archive != 0 ? archive->Reset() : 0;

    if (payload->Init(path) == 0) {
        if (payload != 0) {
            payload->Destroy();
            ::operator delete(payload);
        }

        return 0;
    }

    if (setCurrent != 0) {
        g_zArchive_Current = payload;
    }

    zArchiveList_PushBackPayload(
        g_zArchive_MountedList,
        payload
    );
    return 1;
}
} // namespace zArchive

/**
 * Reimplements 0x48cda0: zReader_AllocateNode (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Allocates a zReader node array and stores the serialized header word in the first node.
 */
extern "C" zReader::Node *__fastcall zReader_AllocateNode(
    int headerWord,
    int fieldCount
) {
    zReader::Node *result = (zReader::Node *)(malloc((size_t)(fieldCount) * sizeof(zReader::Node)));
    result->type = headerWord;
    return result;
}

/**
 * Reimplements 0x4a6110: zReader_ReadString (GameZRecoil/zReader/zreader.cpp).
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
 * Reimplements 0x48d080: zReader_ReadNode (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Reads one serialized zReader node, including recursive array children and scalar/string payloads.
 */
extern "C" int __fastcall zReader_ReadNode(
    void *hFile,
    zReader::Node *outNode
) {
    DWORD bytesRead;
    ReadFile(
        (HANDLE)(hFile),
        outNode,
        4,
        &bytesRead,
        0
    );
    int result = (int)(bytesRead);

    switch (outNode->type) {
    case zReader::ZRDR_NODE_ARRAY: {
        int nodeCount;
        ReadFile(
            (HANDLE)(hFile),
            &nodeCount,
            4,
            &bytesRead,
            0
        );
        result += (int)(bytesRead);

        outNode->value.nodes =
            (zReader::Node *)(malloc((size_t)(nodeCount) * sizeof(zReader::Node)));
        outNode->value.nodes[0].value.i32 = nodeCount;
        outNode->value.nodes[0].type = zReader::ZRDR_NODE_INT;

        nodeCount = 1;
        if (outNode->value.nodes[0].value.i32 > nodeCount) {
            do {
                result += zReader_ReadNode(
                    hFile,
                    &outNode->value.nodes[nodeCount]
                );
                ++nodeCount;
            } while (nodeCount < outNode->value.nodes[0].value.i32);
            return result;
        }

        break;
    }

    case zReader::ZRDR_NODE_INT:
        ReadFile(
            (HANDLE)(hFile),
            &outNode->value,
            4,
            &bytesRead,
            0
        );
        result += (int)(bytesRead);
        break;

    case zReader::ZRDR_NODE_FLOAT:
        ReadFile(
            (HANDLE)(hFile),
            &outNode->value,
            4,
            &bytesRead,
            0
        );
        result += (int)(bytesRead);
        break;

    case zReader::ZRDR_NODE_STRING:
        result += zReader_ReadString(
            hFile,
            &outNode->value
        );
        return result;

    default:
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zReader\\zreader.cpp",
            0x40c,
            "Invalid reader node type in zRdrRead()"
        );
        result = 0;
        break;
    }

    return result;
}

/**
 * Reimplements 0x48ce60: zReader_FreeNodeRecursive (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Recursively releases string and array payload storage owned by a zReader node tree.
 */
extern "C" void __fastcall zReader_FreeNodeRecursive(
    zReader::Node *node
) {
    if (node->type == zReader::ZRDR_NODE_STRING) {
        free(node->value.str);
        node->value.str = 0;
        return;
    }

    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return;
    }

    for (int i = 0; i < node->value.nodes[0].value.i32; ++i) {
        zReader_FreeNodeRecursive(&node->value.nodes[i]);
    }

    free(node->value.nodes);
    node->value.nodes = 0;
}

/**
 * Reimplements 0x48d1c0: zReader_OpenFileFromMountedArchives (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Searches mounted index archives for a member file and returns the first opened handle.
 */
extern "C" void *__fastcall zReader_OpenFileFromMountedArchives(
    const char *path
) {
    if (g_zArchive_MountedList == 0) {
        return INVALID_HANDLE_VALUE;
    }

    int index = 0;
    while (index < zArchiveList_GetCount(g_zArchive_MountedList)) {
        zIndexArchive *archive =
            (zIndexArchive *)(zArchiveList_GetAt(
                g_zArchive_MountedList,
                index
            ));
        void *result = archive->OpenFileByName(
            path,
            0
        );
        if (result != INVALID_HANDLE_VALUE) {
            return result;
        }

        ++index;
    }

    return INVALID_HANDLE_VALUE;
}

namespace zReader {
/**
 * Reimplements 0x4a5c20: zReader::FileExists.
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

/**
 * Reimplements 0x48cd40: zReader::TryResolvePath.
 * Purpose: resolve a filename through mounted archives and search paths.
 */
const char *__fastcall TryResolvePath(
    const char *filename,
    const char *extraSearchPath
) {
    const char *result = 0;
    if (FileExists(filename) != 0) {
        result = filename;
    }

    if (result == 0) {
        if (extraSearchPath != 0 && strlen(extraSearchPath) != 0) {
            zArchiveList *const searchPathList = zUtil_ZRDR_CreateSearchPathList(extraSearchPath);
            result = zUtil_ZRDR_ResolvePathInSearchPathList(
                searchPathList,
                filename
            );
            zUtil_ZRDR_FreeSearchPathList(searchPathList);
        }

        if (result == 0) {
            return zUtil_ZRDR_ResolvePathInSearchPathList(
                g_zRdr_SearchPathList,
                filename
            );
        }
    }

    return result;
}

/**
 * Reimplements 0x421e20: zReader::BuildResolvedParentDir.
 * Purpose: build the parent directory for the currently resolved ZRDR path.
 */
int __fastcall BuildResolvedParentDir(
    const char *filename,
    char *outParentDir
) {
    char fullPath[0x104] = {0};
    _fullpath(
        fullPath,
        TryResolvePath(
            filename,
            0
        ),
        sizeof(fullPath)
    );

    char drive[3] = {0};
    char dir[0x100] = {0};
    char baseName[0x100] = {0};
    char ext[0x100] = {0};
    _splitpath(
        fullPath,
        drive,
        dir,
        baseName,
        ext
    );

    return sprintf(
        outParentDir,
        "%s%s",
        drive,
        dir
    );
}

/**
 * Reimplements 0x48cdc0: zReader::LoadNodeFromPath (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Reduces a path to its basename, opens the mounted archive member, and parses a zReader node tree.
 */
Node *__fastcall LoadNodeFromPath(
    const char *path,
    const char *,
    int
) {
    Node *outNode = 0;
    _splitpath(
        path,
        0,
        0,
        g_zReader_FileNameBuf,
        g_zReader_FileExtBuf
    );
    strcat(
        g_zReader_FileNameBuf,
        g_zReader_FileExtBuf
    );

    void *hFile = zReader_OpenFileFromMountedArchives(g_zReader_FileNameBuf);
    if (hFile != INVALID_HANDLE_VALUE) {
        outNode = zReader_AllocateNode(
            ZRDR_NODE_ARRAY,
            1
        );
        zReader_ReadNode(
            hFile,
            outNode
        );
    }

    return outNode;
}

/**
 * Reimplements 0x48ce40: zReader::FreeLoadedTree (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Releases a loaded zReader tree root and all recursive payload storage.
 */
int __fastcall FreeLoadedTree(
    Node *loaded
) {
    if (loaded != 0) {
        zReader_FreeNodeRecursive(loaded);
        free(loaded);
    }

    return 0;
}

/**
 * Reimplements 0x420be0: zReader::LoadMoversFromZrd.
 * Purpose: load mover definitions from the current ZRD tree.
 */
void LoadMoversFromZrd() {
    Node *const treeRoot = LoadNodeFromPath(
        "movers.zrd",
        0,
        0
    );
    if (treeRoot == 0) {
        return;
    }

    Node *const rootArray = treeRoot->value.nodes;
    Node *const moverArray = rootArray[1].value.nodes;
    const int moverCount = moverArray[0].value.i32 - 1;
    for (int i = 0; i < moverCount; ++i) {
        zClass_NodePartial *const mover = zClass::FindByTypeAndName(
            6,
            moverArray[i + 1].value.str
        );
        if (mover != 0) {
            zClass_Node::PropagateExtraFlagsRecursive(
                mover,
                1
            );
            zClass_Node::SetContextRecursive(
                mover,
                mover,
                0x200000
            );
            g_Mover_LastLoadedNode = mover;
        }
    }

    FreeLoadedTree(treeRoot);
}
} // namespace zReader

/**
 * Reimplements 0x4a5c40: zReader_FileExists_Wrapper.
 * Purpose: expose zReader::FileExists through the original wrapper entry point.
 */
extern "C" int __fastcall zReader_FileExists_Wrapper(
    const char *path
) {
    return zReader::FileExists(path);
}
