#include "zReader.h"

#include "GameZRecoil/zError/zError.h"
#include "zClass.h"

#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>

extern "C" zArchiveList *g_zArchive_MountedList = 0;
extern "C" zArchiveList *g_zArchive_Current = 0;
extern "C" zArchiveList *g_zRdr_SearchPathList = 0;
extern "C" zArchiveList *g_zUtil_ZRDR_FreePool = 0;
extern "C" zArchiveList *g_zRdr_ScratchSearchPathList = 0;
extern "C" int g_zUtil_ZRDR_TotalAllocated = 0;
extern "C" int g_zUtil_ZRDR_FreeCount = 0;
extern "C" int g_zUtil_ZRDR_GrowCount = 0;
extern "C" char g_zReader_FileExtBuf[0x100] = {0};
extern "C" char g_zReader_FileNameBuf[0x100] = {0};
extern "C" char g_zRdr_SplitDriveBuf[0x100] = {0};
extern "C" char g_zRdr_SplitDirBuf[0x100] = {0};
extern "C" char g_zRdr_SplitFileNameBuf[0x100] = {0};
extern "C" char g_zRdr_SplitExtBuf[0x100] = {0};
extern "C" char g_zRdr_PathJoinBuf[0x100] = {0};
extern "C" char g_zRdr_ResolvedPathBuf[0x100] = {0};
extern "C" zClass_NodePartial *g_Mover_LastLoadedNode = 0;

// Reimplements 0x48c9a0: zArchiveList_LinkNodeBetween
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zArchiveList_LinkNodeBetween(
    zArchiveListNode *after, zArchiveListNode *newNode, zArchiveListNode *before) {
    after->next = newNode;
    before->prev = newNode;
    newNode->next = before;
    newNode->prev = after;
}

// Reimplements 0x48c950: zArchiveList_CreateEmpty
extern "C" RECOIL_NOINLINE zArchiveList *RECOIL_CDECL zArchiveList_CreateEmpty() {
    zArchiveList *result = static_cast<zArchiveList *>(malloc(sizeof(zArchiveList)));
    memset(result, 0, sizeof(zArchiveList));
    return result;
}

// Reimplements 0x48c970: zArchiveList_Destroy
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_Destroy(zArchiveList *list) {
    if (list != 0) {
        while (zArchiveList_PopFrontPayload(list) != 0) {
        }

        free(list);
    }

    return 0;
}

// Reimplements 0x48c820: zUtil_ZRDR_PushFreeNode
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zUtil_ZRDR_PushFreeNode(zArchiveListNode *node) {
    if (g_zUtil_ZRDR_FreePool == 0) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    zArchiveListNode *head = g_zUtil_ZRDR_FreePool->head;
    if (head == 0) {
        g_zUtil_ZRDR_FreePool->head = node;
        node->next = node;
        node->prev = node;
    } else {
        zArchiveList_LinkNodeBetween(head->prev, node, head);
        g_zUtil_ZRDR_FreePool->head = node;
    }

    ++g_zUtil_ZRDR_FreePool->count;
    ++g_zUtil_ZRDR_FreeCount;
}

// Reimplements 0x48c800: zUtil_ZRDR_GrowFreePool
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_GrowFreePool() {
    zArchiveListNode *node = static_cast<zArchiveListNode *>(malloc(sizeof(zArchiveListNode)));
    zUtil_ZRDR_PushFreeNode(node);
    ++g_zUtil_ZRDR_TotalAllocated;
}

namespace zUtil {
// Reimplements 0x48c7d0: zUtil::ZRDR_PreallocNodePool
RECOIL_NOINLINE void RECOIL_FASTCALL ZRDR_PreallocNodePool(int count) {
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

// Reimplements 0x48c8e0: zUtil_ZRDR_PopFreeNode
extern "C" RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL
zUtil_ZRDR_PopFreeNode(int allowGrow) {
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

// Reimplements 0x48ca10: zUtil_ZRDR_AllocNodeWithPayload
extern "C" RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL
zUtil_ZRDR_AllocNodeWithPayload(void *payload) {
    zArchiveListNode *result = zUtil_ZRDR_PopFreeNode(1);
    result->payload = payload;
    return result;
}

// Reimplements 0x48c9c0: zArchiveList_PushFrontPayload
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zArchiveList_PushFrontPayload(zArchiveList *list, void *payload) {
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
        zArchiveList_LinkNodeBetween(head->prev, newNode, head);
        list->head = newNode;
    }

    ++list->count;
    return list->count;
}

// Reimplements 0x48ca30: zArchiveList_PushBackPayload
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zArchiveList_PushBackPayload(zArchiveList *list, void *payload) {
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
        zArchiveList_LinkNodeBetween(head->prev, newNode, head);
    }

    ++list->count;
    return list->count;
}

// Reimplements 0x48ca70: zArchiveList_RemovePayload
// (GameZRecoil/zUtil/zutl_zar.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zArchiveList_RemovePayload(zArchiveList *list, void *payload) {
    if (list == 0 || list->count == 0) {
        return -1;
    }

    zArchiveListNode *const node = zArchiveList_FindNodeByPayload(list, payload);
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

// Reimplements 0x48cae0: zArchiveList_FreeNode
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FreeNode(zArchiveListNode *node) {
    if (node == 0) {
        return 0;
    }

    void *payload = node->payload;
    zUtil_ZRDR_PushFreeNode(node);
    return payload;
}

// Reimplements 0x48cb00: zArchiveList_FindNodeByPayload
// (GameZRecoil/zUtil/zutl_zar.cpp)
extern "C" RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL
zArchiveList_FindNodeByPayload(zArchiveList *list, void *payload) {
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

// Reimplements 0x48cb70: zArchiveList_PopFrontPayload
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_PopFrontPayload(zArchiveList *list) {
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

// Reimplements 0x48cc60: zArchiveList_GetCount
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_GetCount(zArchiveList *list) {
    if (list == 0) {
        return 0;
    }

    return list->count;
}

// Reimplements 0x48cb30: zArchiveList_GetAt
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_GetAt(zArchiveList *list,
                                                                    int index) {
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

// Reimplements 0x48cbd0: zArchiveList_FindPayloadByPredicate
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FindPayloadByPredicate(
    zArchiveList *list, zArchiveListPredicate predicate, void *userData) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    if (predicate(node->payload, userData) == 0) {
        goto found;
    }

    while (true) {
        node = node->next;
        if (node == list->head) {
            return 0;
        }

        if (predicate(node->payload, userData) == 0) {
            goto found;
        }
    }

found:
    return node->payload;
}

// Reimplements 0x48cc20: zArchiveList_FindPayloadByValue
// (GameZRecoil/zUtil/zutl_zar.cpp)
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL
zArchiveList_FindPayloadByValue(zArchiveList *list, unsigned int value) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *const head = list->head;
    zArchiveListNode *node = head;
    if (*static_cast<unsigned int *>(node->payload) == value) {
        return node->payload;
    }

    while (true) {
        node = node->next;
        if (node == head) {
            return 0;
        }

        if (*static_cast<unsigned int *>(node->payload) == value) {
            return node->payload;
        }
    }
}

// Reimplements 0x48cc50: zArchiveList_FindPayloadByPredicate_Thunk
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FindPayloadByPredicate_Thunk(
    zArchiveList *list, zArchiveListPredicate predicate, void *userData) {
    return zArchiveList_FindPayloadByPredicate(list, predicate, userData);
}

// Reimplements 0x4a5da0: zUtil_ZRDR_StrCmpPredicate
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_StrCmpPredicate(void *str1,
                                                                                   void *str2) {
    if (str1 == 0 || str2 == 0) {
        return 1;
    }

    return strcmp(static_cast<const char *>(str1), static_cast<const char *>(str2));
}

// Reimplements 0x4a5f20: zUtil_ZRDR_SearchPathContainsFilePredicate
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_SearchPathContainsFilePredicate(void *searchDir, void *filename) {
    sprintf(g_zRdr_PathJoinBuf, "%s\\%s", static_cast<const char *>(searchDir),
                 static_cast<const char *>(filename));
    return zReader::FileExists(g_zRdr_PathJoinBuf) == 0 ? 1 : 0;
}

namespace zUtil {
// Reimplements 0x4a5c50: zUtil::ZRDR_GetFileSize
// (D:\Proj\GameZRecoil\zUtil\zutl_zrdr.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ZRDR_GetFileSize(FILE *fileHandle) {
    if (fileHandle == 0) {
        return 0;
    }

    const int originalOffset = ftell(fileHandle);
    fseek(fileHandle, 0, SEEK_END);
    const int fileSize = ftell(fileHandle);
    fseek(fileHandle, originalOffset, SEEK_SET);
    return fileSize;
}
} // namespace zUtil

// Reimplements 0x4a5e50: zUtil_ZRDR_ResolvePathInSearchPathList
extern "C" RECOIL_NOINLINE char *RECOIL_FASTCALL
zUtil_ZRDR_ResolvePathInSearchPathList(zArchiveList *searchPathList, const char *filename) {
    zArchiveList *list = searchPathList;
    while (true) {
        _splitpath(filename, g_zRdr_SplitDriveBuf, g_zRdr_SplitDirBuf, g_zRdr_SplitFileNameBuf,
                   g_zRdr_SplitExtBuf);
        sprintf(g_zRdr_ResolvedPathBuf, "%s%s", g_zRdr_SplitFileNameBuf, g_zRdr_SplitExtBuf);

        if (list == 0 && g_zRdr_ScratchSearchPathList != 0) {
            list = g_zRdr_ScratchSearchPathList;
        }

        char *matchedDir = static_cast<char *>(zArchiveList_FindPayloadByPredicate(
            list, zUtil_ZRDR_SearchPathContainsFilePredicate, g_zRdr_ResolvedPathBuf));
        if (matchedDir != 0) {
            const size_t len = strlen(matchedDir);
            if (len != 0 && matchedDir[len - 1] == '\\') {
                matchedDir[len - 1] = '\0';
            }

            sprintf(g_zRdr_ResolvedPathBuf, "%s\\%s%s", matchedDir, g_zRdr_SplitFileNameBuf,
                         g_zRdr_SplitExtBuf);
            return g_zRdr_ResolvedPathBuf;
        }

        zArchiveList *scratch = g_zRdr_ScratchSearchPathList;
        if (scratch == 0 || list == scratch) {
            return 0;
        }

        list = 0;
    }
}

// Reimplements 0x4a5f50: zUtil_ZRDR_OpenFileResolved
extern "C" RECOIL_NOINLINE FILE *RECOIL_FASTCALL
zUtil_ZRDR_OpenFileResolved(zArchiveList *searchPathList, const char *filename, const char *mode) {
    char *resolvedPath = zUtil_ZRDR_ResolvePathInSearchPathList(searchPathList, filename);
    return fopen(resolvedPath != 0 ? resolvedPath : filename, mode);
}

namespace zUtil {
// Reimplements 0x4a5ce0: zUtil::ZRDR_AddSearchPaths
RECOIL_NOINLINE void RECOIL_FASTCALL ZRDR_AddSearchPaths(zArchiveList *list, const char *pathText) {
    if (pathText == 0) {
        return;
    }

    zArchiveList *activeList = list;
    while (true) {
        if (activeList != 0) {
            char *copy = _strdup(pathText);
            char *token = strtok(copy, ";");
            while (token != 0) {
                if (zReader_FileExists_Wrapper(token) != 0 &&
                    zArchiveList_FindPayloadByPredicate_Thunk(
                        activeList, zUtil_ZRDR_StrCmpPredicate, token) == 0) {
                    zArchiveList_PushFrontPayload(activeList, _strdup(token));
                }

                token = strtok(0, ";");
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

// Reimplements 0x4a5ca0: zUtil_ZRDR_CreateSearchPathList
extern "C" RECOIL_NOINLINE zArchiveList *RECOIL_FASTCALL
zUtil_ZRDR_CreateSearchPathList(const char *pathText) {
    zArchiveList *list = zArchiveList_CreateEmpty();
    zUtil::ZRDR_AddSearchPaths(list, pathText);
    return list;
}

// Reimplements 0x4a5e10: zUtil_ZRDR_FreePathList
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_FreePathList(zArchiveList *list) {
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

// Reimplements 0x4a5cc0: zUtil_ZRDR_FreeSearchPathList
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_FreeSearchPathList(zArchiveList *list) {
    zUtil_ZRDR_FreePathList(list);
    zArchiveList_Destroy(list);
    return 0;
}

// Reimplements 0x4a5df0: zUtil_ZRDR_FreeScratchSearchPathList
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_FreeScratchSearchPathList() {
    if (g_zRdr_ScratchSearchPathList != 0) {
        zUtil_ZRDR_FreeSearchPathList(g_zRdr_ScratchSearchPathList);
        g_zRdr_ScratchSearchPathList = 0;
    }
}

// Reimplements 0x4a6100: zUtil_ZRDR_ShutdownWildcardPath
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zUtil_ZRDR_ShutdownWildcardPath() {
    zUtil_ZRDR_FreeScratchSearchPathList();
    return 0;
}

namespace {
void DestroyMountedArchive(zIndexArchive *archive) {
    if (archive == 0) {
        return;
    }

    archive->CloseAndFreeRecords();
    archive->Destroy();
    ::operator delete(archive);
}
} // namespace

// Reimplements 0x48d2c0: zUtil_ZRDR_UnloadMountedArchives
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_UnloadMountedArchives(int destroyCurrentToo) {
    zIndexArchive *archive =
        static_cast<zIndexArchive *>(zArchiveList_PopFrontPayload(g_zArchive_MountedList));
    while (archive != 0) {
        zIndexArchive *const current = (zIndexArchive *)(g_zArchive_Current);
        if (destroyCurrentToo != 0 || archive != current) {
            if (current == archive) {
                g_zArchive_Current = 0;
            }

            DestroyMountedArchive(archive);
        }

        archive =
            static_cast<zIndexArchive *>(zArchiveList_PopFrontPayload(g_zArchive_MountedList));
    }

    if (destroyCurrentToo == 0 && g_zArchive_Current != 0) {
        return zArchiveList_PushBackPayload(g_zArchive_MountedList, g_zArchive_Current);
    }

    return 0;
}

// Reimplements 0x48cd10: zUtil_ZRDR_Shutdown
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zUtil_ZRDR_Shutdown() {
    zUtil_ZRDR_FreeSearchPathList(g_zRdr_SearchPathList);
    zUtil_ZRDR_UnloadMountedArchives(1);
    zArchiveList_Destroy(g_zArchive_MountedList);
    g_zRdr_SearchPathList = 0;
    g_zArchive_MountedList = 0;
    return 0;
}

// Reimplements 0x48c890: zUtil_ZRDR_FreeNodePool
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_FreeNodePool() {
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

// Reimplements 0x48cca0: zUtil_ZRDR_SetSearchPath
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_SetSearchPath(const char *pathText) {
    if (g_zRdr_SearchPathList == 0) {
        g_zRdr_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil_ZRDR_FreePathList(g_zRdr_SearchPathList);
    zUtil::ZRDR_AddSearchPaths(g_zRdr_SearchPathList, pathText);
    return 0;
}

// Reimplements 0x48cce0: zUtil_ZRDR_AppendSearchPath
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_AppendSearchPath(const char *pathText) {
    if (g_zRdr_SearchPathList == 0) {
        g_zRdr_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        return 0;
    }

    zUtil::ZRDR_AddSearchPaths(g_zRdr_SearchPathList, pathText);
    return 0;
}

namespace zUtil {
// Reimplements 0x48cc70: zUtil::ZRDR_Init
RECOIL_NOINLINE int RECOIL_FASTCALL ZRDR_Init(const char *pathText) {
    if (g_zArchive_MountedList == 0) {
        g_zArchive_MountedList = zArchiveList_CreateEmpty();
        zUtil_ZRDR_SetSearchPath(pathText);
        g_zArchive_Current = 0;
    }

    return 0;
}
} // namespace zUtil

// Reimplements 0x4a6190: zIndexArchive::Reset
RECOIL_NOINLINE zIndexArchive *RECOIL_THISCALL zIndexArchive::Reset() {
    reservedFree = 0;
    hFile = INVALID_HANDLE_VALUE;
    recordCapacity = 0;
    recordCount = 0;
    records = 0;
    dirty = 0;
    return this;
}

// Reimplements 0x4a61b0: zIndexArchive::Destroy
RECOIL_NOINLINE void RECOIL_THISCALL zIndexArchive::Destroy() {
    CloseAndFreeRecords();
    if (reservedFree != 0) {
        free(reservedFree);
    }
}

// Reimplements 0x4a61d0: zIndexArchive::Init
RECOIL_NOINLINE int RECOIL_THISCALL zIndexArchive::Init(const char *filepath) {
    HANDLE file = CreateFileA(filepath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING,
                              FILE_ATTRIBUTE_NORMAL | FILE_FLAG_RANDOM_ACCESS, 0);

    hFile = file;
    if (file != INVALID_HANDLE_VALUE) {
        return LoadIndexFromTail();
    }

    const DWORD lastError = GetLastError();
    char *message = 0;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, lastError,
                   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)(&message), 0,
                   0);
    zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zUtil\\zutl_zar.cpp", 0x4c,
                      "GetLastError(0x%08x): %s", lastError, message);
    LocalFree(message);
    return 0;
}

// Reimplements 0x4a6270: zIndexArchive::OpenCreateWrite
// (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zIndexArchive::OpenCreateWrite(const char *filepath) {
    HANDLE file = CreateFileA(filepath, GENERIC_READ | GENERIC_WRITE, 0, 0, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, 0);

    hFile = file;
    return file != INVALID_HANDLE_VALUE ? 1 : 0;
}

// Reimplements 0x4a62b0: zIndexArchive::CloseAndFreeRecords
RECOIL_NOINLINE int RECOIL_THISCALL zIndexArchive::CloseAndFreeRecords() {
    if (dirty != 0) {
        FlushIndexToTail();
    }

    HANDLE const file = static_cast<HANDLE>(hFile);
    if (file != INVALID_HANDLE_VALUE) {
        CloseHandle(file);
    }

    hFile = INVALID_HANDLE_VALUE;
    FreeRecordsAndReset();
    return 1;
}

// Reimplements 0x4a6330: zIndexArchive::FreeRecordsAndReset
RECOIL_NOINLINE void RECOIL_THISCALL zIndexArchive::FreeRecordsAndReset() {
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

// Reimplements 0x4a6360: zIndexArchive::FlushIndexToTail
RECOIL_NOINLINE void RECOIL_THISCALL zIndexArchive::FlushIndexToTail() {
    DWORD numberOfBytesWritten = 0;
    const unsigned int footerMagic = 1;
    SetFilePointer(static_cast<HANDLE>(hFile), 0, 0, FILE_END);
    WriteFile(static_cast<HANDLE>(hFile), records, recordCount * sizeof(zZarFileRecord),
              &numberOfBytesWritten, 0);
    WriteFile(static_cast<HANDLE>(hFile), &footerMagic, sizeof(footerMagic), &numberOfBytesWritten,
              0);
    WriteFile(static_cast<HANDLE>(hFile), &recordCount, sizeof(recordCount), &numberOfBytesWritten,
              0);
    dirty = 0;
}

// Reimplements 0x4a63f0: zIndexArchive::LoadIndexFromTail
RECOIL_NOINLINE int RECOIL_THISCALL zIndexArchive::LoadIndexFromTail() {
    HANDLE const file = static_cast<HANDLE>(hFile);
    if (GetFileSize(file, 0) < 8) {
        return 0;
    }

    SetFilePointer(file, -8, 0, FILE_END);

    DWORD numberOfBytesRead = 0;
    unsigned int footerMagic = 0;
    unsigned int recordCountFromTail = 0;
    ReadFile(file, &footerMagic, sizeof(footerMagic), &numberOfBytesRead, 0);
    ReadFile(file, &recordCountFromTail, sizeof(recordCountFromTail), &numberOfBytesRead, 0);

    if (footerMagic != 1) {
        return 0;
    }

    EnsureCapacity(recordCountFromTail);
    const unsigned int bytesToRead = recordCountFromTail * sizeof(zZarFileRecord);
    SetFilePointer(file, -8 - static_cast<LONG>(bytesToRead), 0, FILE_END);
    ReadFile(file, records, bytesToRead, &numberOfBytesRead, 0);
    SetFilePointer(file, 0, 0, FILE_BEGIN);
    recordCount = recordCountFromTail;
    return 1;
}

// Reimplements 0x4a62f0: zIndexArchive::EnsureCapacity
RECOIL_NOINLINE void RECOIL_THISCALL zIndexArchive::EnsureCapacity(unsigned int requiredCount) {
    if (requiredCount < recordCapacity) {
        return;
    }

    unsigned int newCapacity = recordCapacity * 2;
    const unsigned int requiredPlusOne = requiredCount + 1;
    if (newCapacity <= requiredPlusOne) {
        newCapacity = requiredPlusOne;
    }

    recordCapacity = newCapacity;
    records = static_cast<zZarFileRecord *>(
        realloc(records, static_cast<size_t>(newCapacity) * sizeof(zZarFileRecord)));
}

// Reimplements 0x4a64d0: zIndexArchive::AddFileRecord
RECOIL_NOINLINE int RECOIL_THISCALL zIndexArchive::AddFileRecord(
    const char *name, const void *data, unsigned int dataSize, const char *sourceTempPathOrNull,
    const zZarFileTime *sourceFileTimeOrNull) {
    const unsigned int oldRecordCount = recordCount;
    EnsureCapacity(oldRecordCount + 1);

    HANDLE const file = static_cast<HANDLE>(hFile);
    SetFilePointer(file, 0, 0, FILE_END);

    zZarFileRecord record = {0};
    record.fileOffset = GetFileSize(file, 0);
    record.fileSize = dataSize;

    if (sourceTempPathOrNull != 0) {
        record.recordFlags |= 2;
        strncpy(record.sourceTempPath, sourceTempPathOrNull, sizeof(record.sourceTempPath));
        if (sourceFileTimeOrNull != 0) {
            record.sourceFileTimeLow = sourceFileTimeOrNull->lowDateTime;
            record.sourceFileTimeHigh = sourceFileTimeOrNull->highDateTime;
        }
    }

    strncpy(record.name, name, sizeof(record.name));

    DWORD numberOfBytesWritten = 0;
    WriteFile(file, data, dataSize, &numberOfBytesWritten, 0);

    records[oldRecordCount] = record;
    ++recordCount;
    dirty = 1;
    return numberOfBytesWritten == dataSize ? 1 : 0;
}

// Reimplements 0x4a65d0: zIndexArchive::FindRecordByNameCI
RECOIL_NOINLINE zZarFileRecord *RECOIL_THISCALL
zIndexArchive::FindRecordByNameCI(const char *filename) {
    for (unsigned int i = 0; i < recordCount; ++i) {
        zZarFileRecord *record = &records[i];
        if (_stricmp(filename, record->name) == 0) {
            return record;
        }
    }

    return 0;
}

// Reimplements 0x4a6630: zIndexArchive::OpenFileByName
RECOIL_NOINLINE void *RECOIL_THISCALL zIndexArchive::OpenFileByName(const char *filename,
                                                                    unsigned int *outSize) {
    zZarFileRecord *record = FindRecordByNameCI(filename);
    if (record == 0) {
        return INVALID_HANDLE_VALUE;
    }

    if (outSize != 0) {
        *outSize = record->fileSize;
    }

    SetFilePointer(static_cast<HANDLE>(hFile), record->fileOffset, 0, FILE_BEGIN);
    return hFile;
}

// Reimplements 0x4a6670: zIndexArchive::ReadFileByName
RECOIL_NOINLINE int RECOIL_THISCALL
zIndexArchive::ReadFileByName(const char *filename, void *buffer, unsigned int *bufferSize) {
    zZarFileRecord *const record = FindRecordByNameCI(filename);
    if (record == 0) {
        return 0x10001;
    }

    const unsigned int availableBytes = *bufferSize;
    *bufferSize = record->fileSize;
    if (record->fileSize > availableBytes) {
        return 0x10002;
    }

    SetFilePointer(static_cast<HANDLE>(hFile), record->fileOffset, 0, FILE_BEGIN);
    DWORD bytesRead = 0;
    ReadFile(static_cast<HANDLE>(hFile), buffer, record->fileSize, &bytesRead, 0);
    return 0;
}

namespace zArchive {
// Reimplements 0x48d210: zArchive::MountIndexArchive
RECOIL_NOINLINE int RECOIL_FASTCALL MountIndexArchive(const char *path,
                                                               int setCurrent) {
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
        g_zArchive_Current = (zArchiveList *)(payload);
    }

    zArchiveList_PushBackPayload(g_zArchive_MountedList, payload);
    return 1;
}
} // namespace zArchive

// Reimplements 0x48cda0: zReader_AllocateNode
extern "C" RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL
zReader_AllocateNode(int headerWord, int fieldCount) {
    zReader::Node *result = static_cast<zReader::Node *>(
        malloc(static_cast<size_t>(fieldCount) * sizeof(zReader::Node)));
    result->type = headerWord;
    return result;
}

// Reimplements 0x4a6110: zReader_ReadString
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zReader_ReadString(void *hFile, zReader::Value *outString) {
    DWORD bytesRead = 0;
    unsigned int length = 0;
    ReadFile(static_cast<HANDLE>(hFile), &length, 4, &bytesRead, 0);
    int result = static_cast<int>(bytesRead);

    char *buffer = static_cast<char *>(malloc(length + 1));
    outString->str = buffer;
    memset(buffer, 0, length + 1);

    if (static_cast<int>(length) > 0) {
        ReadFile(static_cast<HANDLE>(hFile), outString->str, length, &bytesRead, 0);
        result += static_cast<int>(bytesRead);
    }

    return result;
}

// Reimplements 0x48d080: zReader_ReadNode
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL zReader_ReadNode(void *hFile,
                                                                         zReader::Node *outNode) {
    DWORD bytesRead = 0;
    ReadFile(static_cast<HANDLE>(hFile), outNode, 4, &bytesRead, 0);
    int result = static_cast<int>(bytesRead);

    switch (outNode->type) {
    case zReader::ZRDR_NODE_INT:
    case zReader::ZRDR_NODE_FLOAT:
        ReadFile(static_cast<HANDLE>(hFile), &outNode->value, 4, &bytesRead, 0);
        return result + static_cast<int>(bytesRead);

    case zReader::ZRDR_NODE_STRING:
        return result + zReader_ReadString(hFile, &outNode->value);

    case zReader::ZRDR_NODE_ARRAY: {
        int nodeCount = 0;
        ReadFile(static_cast<HANDLE>(hFile), &nodeCount, 4, &bytesRead, 0);
        result += static_cast<int>(bytesRead);

        zReader::Node *arrayBase = static_cast<zReader::Node *>(
            malloc(static_cast<size_t>(nodeCount) * sizeof(zReader::Node)));
        outNode->value.nodes = arrayBase;
        arrayBase[0].type = zReader::ZRDR_NODE_INT;
        arrayBase[0].value.i32 = nodeCount;

        for (int i = 1; i < nodeCount; ++i) {
            result += zReader_ReadNode(hFile, &arrayBase[i]);
        }

        return result;
    }

    default:
        zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zReader\\zReader.cpp", 0x40c,
                          "Invalid reader node type in zRdrRead()");
        return 0;
    }
}

// Reimplements 0x48ce60: zReader_FreeNodeRecursive
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zReader_FreeNodeRecursive(zReader::Node *node) {
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

// Reimplements 0x48d1c0: zReader_OpenFileFromMountedArchives
extern "C" RECOIL_NOINLINE void *RECOIL_FASTCALL
zReader_OpenFileFromMountedArchives(const char *path) {
    if (g_zArchive_MountedList == 0) {
        return INVALID_HANDLE_VALUE;
    }

    int index = 0;
    while (index < zArchiveList_GetCount(g_zArchive_MountedList)) {
        zIndexArchive *archive =
            static_cast<zIndexArchive *>(zArchiveList_GetAt(g_zArchive_MountedList, index));
        void *result = archive->OpenFileByName(path, 0);
        if (result != INVALID_HANDLE_VALUE) {
            return result;
        }

        ++index;
    }

    return INVALID_HANDLE_VALUE;
}

namespace zReader {
// Reimplements 0x4a5c20: zReader::FileExists
RECOIL_NOINLINE int RECOIL_FASTCALL FileExists(const char *path) {
    return _access(path, 0) == 0 ? 1 : 0;
}

// Reimplements 0x48cd40: zReader::TryResolvePath
RECOIL_NOINLINE const char *RECOIL_FASTCALL TryResolvePath(const char *filename,
                                                           const char *extraSearchPath) {
    const char *result = 0;
    if (FileExists(filename) != 0) {
        result = filename;
    }

    if (result == 0) {
        if (extraSearchPath != 0 && strlen(extraSearchPath) != 0) {
            zArchiveList *const searchPathList = zUtil_ZRDR_CreateSearchPathList(extraSearchPath);
            result = zUtil_ZRDR_ResolvePathInSearchPathList(searchPathList, filename);
            zUtil_ZRDR_FreeSearchPathList(searchPathList);
        }

        if (result == 0) {
            return zUtil_ZRDR_ResolvePathInSearchPathList(g_zRdr_SearchPathList, filename);
        }
    }

    return result;
}

// Reimplements 0x421e20: zReader::BuildResolvedParentDir
RECOIL_NOINLINE int RECOIL_FASTCALL BuildResolvedParentDir(const char *filename,
                                                                    char *outParentDir) {
    char fullPath[0x104] = {0};
    _fullpath(fullPath, TryResolvePath(filename, 0), sizeof(fullPath));

    char drive[3] = {0};
    char dir[0x100] = {0};
    char baseName[0x100] = {0};
    char ext[0x100] = {0};
    _splitpath(fullPath, drive, dir, baseName, ext);

    return sprintf(outParentDir, "%s%s", drive, dir);
}

// Reimplements 0x48cdc0: zReader::LoadNodeFromPath
RECOIL_NOINLINE Node *RECOIL_FASTCALL LoadNodeFromPath(const char *path, const char *,
                                                       int) {
    Node *outNode = 0;
    _splitpath(path, 0, 0, g_zReader_FileNameBuf, g_zReader_FileExtBuf);
    strcat(g_zReader_FileNameBuf, g_zReader_FileExtBuf);

    void *hFile = zReader_OpenFileFromMountedArchives(g_zReader_FileNameBuf);
    if (hFile != INVALID_HANDLE_VALUE) {
        outNode = zReader_AllocateNode(ZRDR_NODE_ARRAY, 1);
        zReader_ReadNode(hFile, outNode);
    }

    return outNode;
}

// Reimplements 0x48ce40: zReader::FreeLoadedTree
RECOIL_NOINLINE int RECOIL_FASTCALL FreeLoadedTree(Node *loaded) {
    if (loaded != 0) {
        zReader_FreeNodeRecursive(loaded);
        free(loaded);
    }

    return 0;
}

// Reimplements 0x420be0: zReader::LoadMoversFromZrd
RECOIL_NOINLINE void RECOIL_CDECL LoadMoversFromZrd() {
    Node *const treeRoot = LoadNodeFromPath("movers.zrd", 0, 0);
    if (treeRoot == 0) {
        return;
    }

    Node *const rootArray = treeRoot->value.nodes;
    Node *const moverArray = rootArray[1].value.nodes;
    const int moverCount = moverArray[0].value.i32 - 1;
    for (int i = 0; i < moverCount; ++i) {
        zClass_NodePartial *const mover = zClass::FindByTypeAndName(6, moverArray[i + 1].value.str);
        if (mover != 0) {
            zClass_Node::PropagateExtraFlagsRecursive(mover, 1);
            zClass_Node::SetContextRecursive(mover, mover, 0x200000);
            g_Mover_LastLoadedNode = mover;
        }
    }

    FreeLoadedTree(treeRoot);
}
} // namespace zReader

// Reimplements 0x4a5c40: zReader_FileExists_Wrapper
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zReader_FileExists_Wrapper(const char *path) {
    return zReader::FileExists(path);
}
