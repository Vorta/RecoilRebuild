#pragma once

#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <stdio.h>

struct zClass_NodePartial;

namespace zReader {
enum NodeType {
    ZRDR_NODE_INT = 1,
    ZRDR_NODE_FLOAT = 2,
    ZRDR_NODE_STRING = 3,
    ZRDR_NODE_ARRAY = 4,
};

struct Node;

union Value {
    unsigned int u32;
    int i32;
    float f32;
    char *str;
    Node *nodes;
    void *ptr;
};

struct Node {
    int type;
    Value value;
};

RECOIL_STATIC_ASSERT(sizeof(Value) == 4);
RECOIL_STATIC_ASSERT(sizeof(Node) == 8);

RECOIL_NOINLINE Node *RECOIL_FASTCALL LoadNodeFromPath(const char *path,
                                                       const char *extraSearchPath = 0,
                                                       int unusedStack = 0);
RECOIL_NOINLINE int RECOIL_FASTCALL FreeLoadedTree(Node *loaded);
RECOIL_NOINLINE const char *RECOIL_FASTCALL ReadNamedString(Node *parentNode, const char *name);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadNamedFloat(Node *parentNode, const char *name,
                                                            float *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadNamedInt(Node *parentNode, const char *name,
                                                          int *outValue);
RECOIL_NOINLINE int RECOIL_FASTCALL FindGlobalStringPrefixIndex(const char *text);
RECOIL_NOINLINE int RECOIL_FASTCALL FileExists(const char *path);
RECOIL_NOINLINE const char *RECOIL_FASTCALL TryResolvePath(const char *filename,
                                                           const char *extraSearchPath);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildResolvedParentDir(const char *filename,
                                                                    char *outParentDir);
RECOIL_NOINLINE void RECOIL_CDECL LoadMoversFromZrd();
} // namespace zReader

extern "C" {
struct zArchiveListNode {
    void *payload;
    zArchiveListNode *next;
    zArchiveListNode *prev;
};

struct zArchiveList {
    int count;
    unsigned int unknown_04;
    unsigned int unknown_08;
    unsigned int unknown_0c;
    zArchiveListNode *head;
};

struct zZarFileRecord {
    unsigned int fileOffset;
    unsigned int fileSize;
    char name[0x40];
    unsigned int recordFlags;
    char sourceTempPath[0x40];
    unsigned int sourceFileTimeLow;
    unsigned int sourceFileTimeHigh;
};

struct zZarFileTime {
    unsigned int lowDateTime;
    unsigned int highDateTime;
};

struct zIndexArchive {
    void *reservedFree;
    void *hFile;
    unsigned int dirty;
    unsigned int recordCount;
    unsigned int recordCapacity;
    zZarFileRecord *records;

    RECOIL_NOINLINE zIndexArchive *RECOIL_THISCALL Reset();
    RECOIL_NOINLINE void RECOIL_THISCALL Destroy();
    RECOIL_NOINLINE int RECOIL_THISCALL Init(const char *filepath);
    RECOIL_NOINLINE int RECOIL_THISCALL OpenCreateWrite(const char *filepath);
    RECOIL_NOINLINE int RECOIL_THISCALL CloseAndFreeRecords();
    RECOIL_NOINLINE void RECOIL_THISCALL FreeRecordsAndReset();
    RECOIL_NOINLINE void RECOIL_THISCALL FlushIndexToTail();
    RECOIL_NOINLINE int RECOIL_THISCALL LoadIndexFromTail();
    RECOIL_NOINLINE void RECOIL_THISCALL EnsureCapacity(unsigned int requiredCount);
    RECOIL_NOINLINE int RECOIL_THISCALL
    AddFileRecord(const char *name, const void *data, unsigned int dataSize,
                  const char *sourceTempPathOrNull, const zZarFileTime *sourceFileTimeOrNull);
    RECOIL_NOINLINE zZarFileRecord *RECOIL_THISCALL FindRecordByNameCI(const char *filename);
    RECOIL_NOINLINE void *RECOIL_THISCALL OpenFileByName(const char *filename,
                                                         unsigned int *outSize);
    RECOIL_NOINLINE int RECOIL_THISCALL ReadFileByName(const char *filename, void *buffer,
                                                                unsigned int *bufferSize);
};

RECOIL_STATIC_ASSERT(sizeof(zZarFileRecord) == 0x94);
RECOIL_STATIC_ASSERT(sizeof(zIndexArchive) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zArchiveListNode) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zArchiveList) == 0x14);

extern zArchiveList *g_zArchive_MountedList;
extern zArchiveList *g_zArchive_Current;
extern zArchiveList *g_zRdr_SearchPathList;
extern zArchiveList *g_zUtil_ZRDR_FreePool;
extern zArchiveList *g_zRdr_ScratchSearchPathList;
extern int g_zUtil_ZRDR_TotalAllocated;
extern int g_zUtil_ZRDR_FreeCount;
extern int g_zUtil_ZRDR_GrowCount;
extern char g_zReader_FileExtBuf[0x100];
extern char g_zReader_FileNameBuf[0x100];
extern zClass_NodePartial *g_Mover_LastLoadedNode;
extern int g_zRndr_GlobalStringCount;
extern char *g_zRndr_GlobalStringTable[100];

typedef int (RECOIL_FASTCALL *zArchiveListPredicate)(void *, void *);

RECOIL_NOINLINE zArchiveList *RECOIL_CDECL zArchiveList_CreateEmpty();
RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_Destroy(zArchiveList *list);
RECOIL_NOINLINE void RECOIL_FASTCALL zArchiveList_LinkNodeBetween(zArchiveListNode *after,
                                                                  zArchiveListNode *newNode,
                                                                  zArchiveListNode *before);
RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_PushFrontPayload(zArchiveList *list,
                                                                           void *payload);
RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_PushBackPayload(zArchiveList *list,
                                                                          void *payload);
RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_RemovePayload(zArchiveList *list,
                                                                        void *payload);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FreeNode(zArchiveListNode *node);
RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL zArchiveList_FindNodeByPayload(zArchiveList *list,
                                                                                 void *payload);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_PopFrontPayload(zArchiveList *list);
RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL zUtil_ZRDR_AllocNodeWithPayload(void *payload);
RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_SetSearchPath(const char *pathText);
RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_AppendSearchPath(const char *pathText);
RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_FreePathList(zArchiveList *list);
RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_FreeSearchPathList(zArchiveList *list);
RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_FreeScratchSearchPathList();
RECOIL_NOINLINE int RECOIL_CDECL zUtil_ZRDR_ShutdownWildcardPath();
RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_UnloadMountedArchives(int destroyCurrentToo);
RECOIL_NOINLINE int RECOIL_CDECL zUtil_ZRDR_Shutdown();
RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_FreeNodePool();
RECOIL_NOINLINE void RECOIL_CDECL zUtil_ZRDR_GrowFreePool();
RECOIL_NOINLINE void RECOIL_FASTCALL zUtil_ZRDR_PushFreeNode(zArchiveListNode *node);
RECOIL_NOINLINE zArchiveListNode *RECOIL_FASTCALL zUtil_ZRDR_PopFreeNode(int allowGrow);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FindPayloadByPredicate(
    zArchiveList *list, zArchiveListPredicate predicate, void *userData);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FindPayloadByValue(zArchiveList *list,
                                                                      unsigned int value);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_FindPayloadByPredicate_Thunk(
    zArchiveList *list, zArchiveListPredicate predicate, void *userData);
RECOIL_NOINLINE int RECOIL_FASTCALL zUtil_ZRDR_StrCmpPredicate(void *str1, void *str2);
RECOIL_NOINLINE int RECOIL_FASTCALL
zUtil_ZRDR_SearchPathContainsFilePredicate(void *searchDir, void *filename);
RECOIL_NOINLINE char *RECOIL_FASTCALL
zUtil_ZRDR_ResolvePathInSearchPathList(zArchiveList *searchPathList, const char *filename);
RECOIL_NOINLINE FILE *RECOIL_FASTCALL zUtil_ZRDR_OpenFileResolved(zArchiveList *searchPathList,
                                                                       const char *filename,
                                                                       const char *mode);
RECOIL_NOINLINE zArchiveList *RECOIL_FASTCALL zUtil_ZRDR_CreateSearchPathList(const char *pathText);
RECOIL_NOINLINE int RECOIL_FASTCALL zReader_FileExists_Wrapper(const char *path);
RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL zReader_FindChildRecursive(zReader::Node *node,
                                                                          const char *searchName,
                                                                          int startIndex);
RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL zReader_GetNamedNode(zReader::Node *parentNode,
                                                                    const char *name);
RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL zReader_AllocateNode(int headerWord,
                                                                    int fieldCount);
RECOIL_NOINLINE int RECOIL_FASTCALL zReader_ReadString(void *hFile,
                                                                zReader::Value *outString);
RECOIL_NOINLINE int RECOIL_FASTCALL zReader_ReadNode(void *hFile, zReader::Node *outNode);
RECOIL_NOINLINE void RECOIL_FASTCALL zReader_FreeNodeRecursive(zReader::Node *node);
RECOIL_NOINLINE void *RECOIL_FASTCALL zReader_OpenFileFromMountedArchives(const char *path);
RECOIL_NOINLINE int RECOIL_FASTCALL zArchiveList_GetCount(zArchiveList *list);
RECOIL_NOINLINE void *RECOIL_FASTCALL zArchiveList_GetAt(zArchiveList *list, int index);
}

namespace zUtil {
RECOIL_NOINLINE void RECOIL_FASTCALL ZRDR_PreallocNodePool(int count);
RECOIL_NOINLINE int RECOIL_FASTCALL ZRDR_Init(const char *pathText);
RECOIL_NOINLINE int RECOIL_FASTCALL ZRDR_GetFileSize(FILE *fileHandle);
RECOIL_NOINLINE void RECOIL_FASTCALL ZRDR_AddSearchPaths(zArchiveList *list, const char *pathText);
} // namespace zUtil

namespace zArchive {
RECOIL_NOINLINE int RECOIL_FASTCALL MountIndexArchive(const char *path,
                                                               int setCurrent);
}
