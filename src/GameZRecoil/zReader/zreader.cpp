#include "zreader.h"

#include "GameZRecoil/zError/zerr.h"

#include <windows.h>

#include <io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Purpose: store reusable archive-list nodes for ZRDR list operations.
 */
extern "C" zArchiveList *g_zUtil_ZRDR_FreePool = 0;
/**
 * Purpose: count archive-list nodes allocated for the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_TotalAllocated = 0;
/**
 * Purpose: count nodes currently held by the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_FreeCount = 0;
/**
 * Purpose: count demand-growth events for the ZRDR free pool.
 */
extern "C" int g_zUtil_ZRDR_GrowCount = 0;
/**
 * Purpose: store the file extension split out before archive-member lookup.
 */
extern "C" char g_zReader_FileExtBuf[0x100] = {0};
/**
 * Purpose: store the basename joined with extension for archive-member lookup.
 */
extern "C" char g_zReader_FileNameBuf[0x100] = {0};
/**
 * Purpose: store the global ZRDR search-path list.
 */
extern "C" zArchiveList *g_zRdr_SearchPathList = 0;
/**
 * Purpose: store the process-wide list of mounted ZRDR/ZAR index archives.
 */
extern "C" zArchiveList *g_zArchive_MountedList = 0;
/**
 * Purpose: store the current mounted archive selected by archive mounting.
 */
extern "C" zIndexArchive *g_zArchive_Current = 0;
namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zrdr-preallocnodepool
 * @recoil-artifact defines .text recoil:function:0x48c7d0: zUtil::ZRDR_InitNodePool.
 * @recoil-match byte
 *
 * Purpose: initialize an absent free-node pool with the requested node count.
 */
void __fastcall ZRDR_InitNodePool(
    int count
) {
    if (g_zUtil_ZRDR_FreePool != 0) {
        return;
    }

    g_zUtil_ZRDR_FreePool = zArchiveList_New();
    while (count > 0) {
        zUtil_ZRDR_GrowNodePool();
        --count;
    }
}
} // namespace zUtil


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-growfreepool
 * @recoil-artifact defines .text recoil:function:0x48c800: zUtil_ZRDR_GrowNodePool.
 * @recoil-match byte
 *
 * Purpose: allocate one reusable ZRDR list node and add it to the free pool.
 */
extern "C" void __cdecl zUtil_ZRDR_GrowNodePool() {
    zArchiveListNode *node = (zArchiveListNode *)(malloc(sizeof(zArchiveListNode)));
    zUtil_ZRDR_FreeNode(node);
    ++g_zUtil_ZRDR_TotalAllocated;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-pushfreenode
 * @recoil-artifact defines .text recoil:function:0x48c820: zUtil_ZRDR_FreeNode.
 * @recoil-match byte
 *
 * Purpose: return a node to the shared ZRDR free-node list.
 */
extern "C" void __fastcall zUtil_ZRDR_FreeNode(
    zArchiveListNode *node
) {
    zArchiveList *pool = g_zUtil_ZRDR_FreePool;
    if (g_zUtil_ZRDR_FreePool == 0) {
        pool = g_zUtil_ZRDR_FreePool = zArchiveList_New();
    }

    zArchiveListNode **headPtr = &g_zUtil_ZRDR_FreePool->head;
    zArchiveListNode *head = *headPtr;
    if (head == 0) {
        *headPtr = node;
        node->next = node;
        node->prev = node;
    } else {
        zArchiveList_Link(
            head->prev,
            node,
            head
        );
        *headPtr = node;
    }

    ++pool->count;
    ++g_zUtil_ZRDR_FreeCount;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-freenodepool
 * @recoil-artifact defines .text recoil:function:0x48c890: zUtil_ZRDR_FreeNodePool.
 * @recoil-match byte
 *
 * Purpose: release all nodes currently held in the ZRDR free-node pool.
 */
extern "C" void __cdecl zUtil_ZRDR_FreeNodePool() {
    if (g_zUtil_ZRDR_FreePool == 0) {
        return;
    }

    zArchiveListNode *node = zUtil_ZRDR_AllocNode(0);
    while (node != 0) {
        free(node);
        node = zUtil_ZRDR_AllocNode(0);
    }

    zArchiveList_Free(g_zUtil_ZRDR_FreePool);
    g_zUtil_ZRDR_FreePool = 0;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-popfreenode
 * @recoil-artifact defines .text recoil:function:0x48c8e0: zUtil_ZRDR_AllocNode.
 * @recoil-match byte
 *
 * Purpose: pop one reusable node from the ZRDR free-node list.
 */
extern "C" zArchiveListNode *__fastcall zUtil_ZRDR_AllocNode(
    int allowGrow
) {
    zArchiveList *pool = g_zUtil_ZRDR_FreePool;
    if (pool->count == 0) {
        if (allowGrow != 0) {
            zUtil_ZRDR_GrowNodePool();
            ++g_zUtil_ZRDR_GrowCount;
        } else {
            return 0;
        }
    }
    zArchiveListNode *head = pool->head;
    if (pool->count == 1) {
        pool->head = 0;
    } else {
        head->prev->next = head->next;
        head->next->prev = head->prev;
        pool->head = head->next;
    }
    pool->count--;
    g_zUtil_ZRDR_FreeCount--;
    return head;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-createempty
 * @recoil-artifact defines .text recoil:function:0x48c950: zArchiveList_New.
 * @recoil-match byte
 *
 * Purpose: allocate and initialize an empty circular archive list.
 */
extern "C" zArchiveList *__cdecl zArchiveList_New() {
    zArchiveList *result = (zArchiveList *)(malloc(sizeof(zArchiveList)));
    result->unknown_04 = 0;
    result->count = 0;
    result->unknown_08 = 0;
    result->unknown_0c = 0;
    result->head = 0;
    return result;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-destroy
 * @recoil-artifact defines .text recoil:function:0x48c970: zArchiveList_Free.
 * @recoil-match byte
 *
 * Purpose: drain an archive-list container and release the list allocation.
 */
extern "C" int __fastcall zArchiveList_Free(
    zArchiveList *list
) {
    if (list != 0) {
        while (zArchiveList_RemoveHead(list) != 0) {
        }

        free(list);
    }

    return 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-linknodebetween
 * @recoil-artifact defines .text recoil:function:0x48c9a0: zArchiveList_Link.
 * @recoil-match byte
 *
 * Purpose: link a node between two existing circular-list neighbors.
 */
extern "C" void __fastcall zArchiveList_Link(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-pushfrontpayload
 * @recoil-artifact defines .text recoil:function:0x48c9c0: zArchiveList_AddHead.
 * @recoil-match byte
 *
 * Purpose: insert a payload node at the head of an archive list.
 */
extern "C" int __fastcall zArchiveList_AddHead(
    zArchiveList *list,
    void *payload
) {
    if (list == 0) {
        return -1;
    }

    zArchiveListNode *newNode = zArchiveList_AllocNode(payload);
    zArchiveListNode *head = list->head;
    if (head == 0) {
        list->head = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        zArchiveList_Link(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-allocnodewithpayload
 * @recoil-artifact defines .text recoil:function:0x48ca10: zArchiveList_AllocNode.
 * @recoil-match byte
 *
 * Purpose: allocate a list node and attach the supplied payload pointer.
 */
extern "C" zArchiveListNode *__fastcall zArchiveList_AllocNode(
    void *payload
) {
    zArchiveListNode *result = zUtil_ZRDR_AllocNode(1);
    result->payload = payload;
    return result;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-pushbackpayload
 * @recoil-artifact defines .text recoil:function:0x48ca30: zArchiveList_AddTail.
 * @recoil-match byte
 *
 * Purpose: insert a payload node at the tail of an archive list.
 */
extern "C" int __fastcall zArchiveList_AddTail(
    zArchiveList *list,
    void *payload
) {
    if (list == 0) {
        return -1;
    }

    zArchiveListNode *newNode = zArchiveList_AllocNode(payload);
    zArchiveListNode *head = list->head;
    if (head == 0) {
        list->head = newNode;
        newNode->next = newNode;
        newNode->prev = newNode;
    } else {
        zArchiveList_Link(
            head->prev,
            newNode,
            head
        );
    }

    ++list->count;
    return list->count;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-removepayload
 * @recoil-artifact defines .text recoil:function:0x48ca70: zArchiveList_Remove.
 * @recoil-match byte
 *
 * Purpose: remove a matching payload node from a circular archive list and
 * return the remaining node count.
 */
extern "C" int __fastcall zArchiveList_Remove(
    zArchiveList *list,
    void *payload
) {
    if (list == 0) {
        return -1;
    }
    if (list->count == 0) {
        return -1;
    }

    zArchiveListNode *const node = zArchiveList_FindNode(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-freenode
 * @recoil-artifact defines .text recoil:function:0x48cae0: zArchiveList_FreeNode.
 * @recoil-match byte
 *
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
    zUtil_ZRDR_FreeNode(node);
    return payload;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-findnodebypayload
 * @recoil-artifact defines .text recoil:function:0x48cb00: zArchiveList_FindNode.
 * @recoil-match byte
 *
 * Purpose: find the circular-list node that owns a specific payload pointer.
 */
extern "C" zArchiveListNode *__fastcall zArchiveList_FindNode(
    zArchiveList *list,
    void *payload
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    zArchiveListNode *const head = list->head;
    while (node->payload != payload) {
        node = node->next;
        if (node == head) {
            return 0;
        }
    }
    return node;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-getat
 * @recoil-artifact defines .text recoil:function:0x48cb30: zArchiveList_Get.
 * @recoil-match byte
 *
 * Purpose: return the payload at a zero-based archive-list index.
 */
extern "C" void *__fastcall zArchiveList_Get(
    zArchiveList *list,
    int index
) {
    if (list == 0) {
        return 0;
    }
    if (index >= list->count) {
        return 0;
    }

    if (index == 0) {
        return list->head->payload;
    }

    int i = 1;
    zArchiveListNode *head = list->head;
    zArchiveListNode *node = head->next;
    while (node != head && i != index) {
        node = node->next;
        ++i;
    }

    return node->payload;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-popfrontpayload
 * @recoil-artifact defines .text recoil:function:0x48cb70: zArchiveList_RemoveHead.
 * @recoil-match byte
 *
 * Purpose: unlink the head node from an archive list and return its payload.
 */
extern "C" void *__fastcall zArchiveList_RemoveHead(
    zArchiveList *list
) {
    if (list == 0) {
        return 0;
    }

    if (list->count == 0) {
        return 0;
    }

    zArchiveListNode *head = list->head;
    if (list->count == 1) {
        list->head = 0;
    } else {
        head->prev->next = head->next;
        head->next->prev = head->prev;
        list->head = head->next;
    }
    list->count--;
    return zArchiveList_FreeNode(head);
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-findpayloadbypredicate
 * @recoil-artifact defines .text recoil:function:0x48cbd0: zArchiveList_FindCompare.
 * @recoil-match byte
 *
 * Purpose: find the first item for which the caller-supplied comparison returns zero.
 */
extern "C" void *__fastcall zArchiveList_FindCompare(
    zArchiveList *list,
    zArchiveListCompare compare,
    void *userData
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    while (compare(node->payload, userData) != 0) {
        node = node->next;
        if (node == list->head) {
            return 0;
        }
    }
    return node->payload;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-findpayloadbyvalue
 * @recoil-artifact defines .text recoil:function:0x48cc20: zArchiveList_FindKey.
 * @recoil-match byte
 *
 * Purpose: find the first payload whose leading dword equals the requested
 * value.
 */
extern "C" void *__fastcall zArchiveList_FindKey(
    zArchiveList *list,
    unsigned int value
) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    zArchiveListNode *const head = list->head;
    while (*(unsigned int *)(node->payload) != value) {
        node = node->next;
        if (node == head) {
            return 0;
        }
    }
    return node->payload;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-findpayloadbypredicate-thunk
 * @recoil-artifact defines .text recoil:function:0x48cc50: zArchiveList_Find.
 * @recoil-match byte
 *
 * Purpose: expose the comparison-based list search through the public entry point.
 */
extern "C" void *__fastcall zArchiveList_Find(
    zArchiveList *list,
    zArchiveListCompare compare,
    void *userData
) {
    return zArchiveList_FindCompare(
        list,
        compare,
        userData
    );
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zarchivelist-getcount
 * @recoil-artifact defines .text recoil:function:0x48cc60: zArchiveList_Count.
 * @recoil-match byte
 *
 * Purpose: return the number of payload nodes in an archive list.
 */
extern "C" int __fastcall zArchiveList_Count(
    zArchiveList *list
) {
    if (list == 0) {
        return 0;
    }

    return list->count;
}


namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zrdr-init
 * @recoil-artifact defines .text recoil:function:0x48cc70: zUtil::ZRDR_Init.
 * @recoil-match byte
 *
 * Purpose: initialize ZRDR search-path and node-pool state.
 */
int __fastcall ZRDR_Init(
    const char *pathText
) {
    if (g_zArchive_MountedList == 0) {
        g_zArchive_MountedList = zArchiveList_New();
        zUtil_ZRDR_SetPath(pathText);
        g_zArchive_Current = 0;
    }

    return 0;
}
} // namespace zUtil


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-setsearchpath
 * @recoil-artifact defines .text recoil:function:0x48cca0: zUtil_ZRDR_SetPath.
 * @recoil-match byte
 *
 * Purpose: replace the current ZRDR search path list.
 */
extern "C" int __fastcall zUtil_ZRDR_SetPath(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-appendsearchpath
 * @recoil-artifact defines .text recoil:function:0x48cce0: zUtil_ZRDR_AddPath.
 * @recoil-match byte
 *
 * Purpose: append additional paths to the current ZRDR search path list.
 */
extern "C" int __fastcall zUtil_ZRDR_AddPath(
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


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-shutdown
 * @recoil-artifact defines .text recoil:function:0x48cd10: zUtil_ZRDR_Exit.
 * @recoil-match byte
 *
 * Purpose: shut down ZRDR path state, mounted archives, and node pools.
 */
extern "C" int __cdecl zUtil_ZRDR_Exit() {
    zUtil_ZRDR_FreeSearchPathList(g_zRdr_SearchPathList);
    zUtil_ZRDR_Unmount(1);
    zArchiveList_Free(g_zArchive_MountedList);
    g_zRdr_SearchPathList = 0;
    g_zArchive_MountedList = 0;
    return 0;
}
namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-tryresolvepath
 * @recoil-artifact defines .text recoil:function:0x48cd40: zReader::FindFile.
 * @recoil-match byte
 *
 * Purpose: resolve a filename through mounted archives and search paths.
 */
const char *__fastcall FindFile(
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
            result = zUtil_ZRDR_ResolvePathInSearchPathList(
                g_zRdr_SearchPathList,
                filename
            );
        }
    }

    return result;
}
} // namespace zReader


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-allocatenode
 * @recoil-artifact defines .text recoil:function:0x48cda0: zRdrAllocNode (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Allocates a zReader node array and stores the serialized header word in the first node.
 */
extern "C" zReader::Node *__fastcall zRdrAllocNode(
    int headerWord,
    int fieldCount
) {
    zReader::Node *result = (zReader::Node *)(malloc((size_t)(fieldCount) * sizeof(zReader::Node)));
    result->type = headerWord;
    return result;
}
namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-loadnodefrompath
 * @recoil-artifact defines .text recoil:function:0x48cdc0: zReader::Load (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Reduces a path to its basename, opens the mounted archive member, and parses a zReader node tree.
 */
Node *__fastcall Load(
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

    void *hFile = zRdrOpenFile(g_zReader_FileNameBuf);
    if (hFile != INVALID_HANDLE_VALUE) {
        outNode = zRdrAllocNode(
            ZRDR_NODE_ARRAY,
            1
        );
        zRdrRead(
            hFile,
            outNode
        );
    }

    return outNode;
}
} // namespace zReader


namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-freeloadedtree
 * @recoil-artifact defines .text recoil:function:0x48ce40: zReader::Free (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Releases a loaded zReader tree root and all recursive payload storage.
 */
int __fastcall Free(
    Node *loaded
) {
    if (loaded != 0) {
        zRdrFreeContents(loaded);
        free(loaded);
    }

    return 0;
}
} // namespace zReader


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-freenoderecursive
 * @recoil-artifact defines .text recoil:function:0x48ce60: zRdrFreeContents (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: free string and array contents recursively, leaving the supplied node allocated.
 */
extern "C" void __fastcall zRdrFreeContents(
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
        zRdrFreeContents(&node->value.nodes[i]);
    }

    free(node->value.nodes);
    node->value.nodes = 0;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-findchildrecursive
 * @recoil-artifact defines .text recoil:function:0x48cec0: zRdrFindNode (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Recursively finds a named zReader child and returns the value node adjacent to the matching name string.
 */
extern "C" zReader::Node *__fastcall zRdrFindNode(
    zReader::Node *node,
    const char *searchName,
    int startIndex
) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *arrayBase = node->value.nodes;
    int index = startIndex;
    if (startIndex >= arrayBase->value.i32) {
        return 0;
    }

    while (index < node->value.nodes->value.i32) {
        zReader::Node *child = &node->value.nodes[index];
        int childType = child->type;
        if (childType == zReader::ZRDR_NODE_ARRAY) {
            zReader::Node *result = zRdrFindNode(
                child,
                searchName,
                1
            );
            if (result != 0) {
                return result;
            }
        } else if (childType == zReader::ZRDR_NODE_STRING &&
                   strcmp(
                       child->value.str,
                       searchName
                   ) == 0) {
            return &child[1];
        }

        ++index;
    }

    return 0;
}


/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-getnamednode
 * @recoil-artifact defines .text recoil:function:0x48cf70: zRdrGetNode (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Starts the recursive named-node lookup at the first payload child of an array node.
 */
extern "C" zReader::Node *__fastcall zRdrGetNode(
    zReader::Node *parentNode,
    const char *name
) {
    return zRdrFindNode(
        parentNode,
        name,
        1
    );
}


namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-readnamedstring
 * @recoil-artifact defines .text recoil:function:0x48cf80: zReader::GetString.
 * @recoil-match byte
 *
 * Purpose: read a named string value from a node or the first payload item of a
 * named array node.
 */
const char *__fastcall GetString(
    Node *parentNode,
    const char *name
) {
    Node *node = zRdrGetNode(
        parentNode,
        name
    );
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_STRING) {
        return node->value.str;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_STRING) {
            return arrayBase[1].value.str;
        }
    }

    return 0;
}
} // namespace zReader

namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-readnamedfloat
 * @recoil-artifact defines .text recoil:function:0x48cfb0: zReader::GetFloat.
 * @recoil-match byte
 *
 * Purpose: read a named float value, accepting integer nodes as float-compatible
 * values when the source data stores the number as an int.
 */
int __fastcall GetFloat(
    Node *parentNode,
    const char *name,
    float *outValue
) {
    Node *node = zRdrGetNode(
        parentNode,
        name
    );
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_FLOAT) {
        *outValue = node->value.f32;
        return 1;
    }

    if (node->type == ZRDR_NODE_INT) {
        *outValue = (float)(node->value.i32);
        return 1;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_FLOAT) {
            *outValue = arrayBase[1].value.f32;
            return 1;
        }

    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_INT) {
            *outValue = (float)(arrayBase[1].value.i32);
            return 1;
        }
    }

    return 0;
}
} // namespace zReader


namespace zReader {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-readnamedint
 * @recoil-artifact defines .text recoil:function:0x48d030: zReader::GetInt.
 * @recoil-match byte
 *
 * Purpose: read a named integer value from a node or the first payload item of
 * a named array node.
 */
int __fastcall GetInt(
    Node *parentNode,
    const char *name,
    int *outValue
) {
    Node *node = zRdrGetNode(
        parentNode,
        name
    );
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_INT) {
        *outValue = node->value.i32;
        return 1;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_INT) {
            *outValue = arrayBase[1].value.i32;
            return 1;
        }
    }

    return 0;
}
} // namespace zReader
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-readnode
 * @recoil-artifact defines .text recoil:function:0x48d080: zRdrRead (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Reads one serialized zReader node, including recursive array children and scalar/string payloads.
 */
extern "C" int __fastcall zRdrRead(
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
                result += zRdrRead(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zreader-openfilefrommountedarchives
 * @recoil-artifact defines .text recoil:function:0x48d1c0: zRdrOpenFile (GameZRecoil/zReader/zreader.cpp).
 * @recoil-match byte
 *
 * Purpose: Searches mounted index archives for a member file and returns the first opened handle.
 */
extern "C" void *__fastcall zRdrOpenFile(
    const char *path
) {
    if (g_zArchive_MountedList == 0) {
        return INVALID_HANDLE_VALUE;
    }

    int index = 0;
    while (index < zArchiveList_Count(g_zArchive_MountedList)) {
        zIndexArchive *archive =
            (zIndexArchive *)(zArchiveList_Get(
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
namespace zArchive {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-mountindexarchive
 * @recoil-artifact defines .text recoil:function:0x48d210: zArchive::Mount.
 * @recoil-match byte
 *
 * Purpose: allocate, initialize, and register a mounted index archive.
 */
int __fastcall Mount(
    const char *path,
    int setCurrent
) {
    zIndexArchive *payload = new zIndexArchive;

    if (payload->Init(path) != 0) {
        if (setCurrent != 0) {
            g_zArchive_Current = payload;
        }

        zArchiveList_AddTail(
            g_zArchive_MountedList,
            payload
        );
        return 1;
    }

    delete payload;

    return 0;
}
} // namespace zArchive
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zreader-zreader-zutil-zrdr-unloadmountedarchives
 * @recoil-artifact defines .text recoil:function:0x48d2c0: zUtil_ZRDR_Unmount.
 * @recoil-match byte
 *
 * Purpose: destroy mounted archives while optionally preserving the current archive.
 */
extern "C" void __fastcall zUtil_ZRDR_Unmount(
    int destroyCurrentToo
) {
    zIndexArchive *archive =
        (zIndexArchive *)(zArchiveList_RemoveHead(g_zArchive_MountedList));
    while (archive != 0) {
        zIndexArchive *const current = g_zArchive_Current;
        if (destroyCurrentToo != 0 || archive != current) {
            if (current == archive) {
                g_zArchive_Current = 0;
            }

            archive->CloseAndFreeRecords();
            delete archive;
        }

        archive = (zIndexArchive *)(zArchiveList_RemoveHead(g_zArchive_MountedList));
    }

    if (destroyCurrentToo == 0 && g_zArchive_Current != 0) {
        zArchiveList_AddTail(
            g_zArchive_MountedList,
            g_zArchive_Current
        );
    }


}
