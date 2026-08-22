#pragma once
#ifndef GAMEZRECOIL_ZREADER_ZREADER_H
#define GAMEZRECOIL_ZREADER_ZREADER_H

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

Node *__fastcall LoadNodeFromPath(
    const char *path,
    const char *extraSearchPath = 0,
    int unusedStack = 0
);
int __fastcall FreeLoadedTree(Node *loaded);
const char *__fastcall ReadNamedString(
    Node *parentNode,
    const char *name
);
int __fastcall ReadNamedFloat(
    Node *parentNode,
    const char *name,
    float *outValue
);
int __fastcall ReadNamedInt(
    Node *parentNode,
    const char *name,
    int *outValue
);
int __fastcall FindGlobalStringPrefixIndex(const char *text);
int __fastcall FileExists(const char *path);
const char *__fastcall TryResolvePath(
    const char *filename,
    const char *extraSearchPath
);
int __fastcall BuildResolvedParentDir(
    const char *filename,
    char *outParentDir
);
void __cdecl LoadMoversFromZrd();
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

    zIndexArchive * Reset();
    void Destroy();
    int Init(const char *filepath);
    int OpenCreateWrite(const char *filepath);
    int CloseAndFreeRecords();
    void FreeRecordsAndReset();
    void FlushIndexToTail();
    int LoadIndexFromTail();
    void EnsureCapacity(unsigned int requiredCount);
    int AddFileRecord(
        const char *name,
        const void *data,
        unsigned int dataSize,
        const char *sourceTempPathOrNull,
        const zZarFileTime *sourceFileTimeOrNull
    );
    zZarFileRecord * FindRecordByNameCI(const char *filename);
    void * OpenFileByName(
        const char *filename,
        unsigned int *outSize
    );
    int ReadFileByName(
        const char *filename,
        void *buffer,
        unsigned int *bufferSize
    );
};

RECOIL_STATIC_ASSERT(sizeof(zZarFileRecord) == 0x94);
RECOIL_STATIC_ASSERT(sizeof(zIndexArchive) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(zArchiveListNode) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zArchiveList) == 0x14);

extern zArchiveList *g_zArchive_MountedList;
extern zIndexArchive *g_zArchive_Current;
extern zArchiveList *g_zRdr_SearchPathList;
extern zArchiveList *g_zUtil_ZRDR_FreePool;
extern zArchiveList *g_zRdr_ScratchSearchPathList;
extern int g_zUtil_ZRDR_TotalAllocated;
extern int g_zUtil_ZRDR_FreeCount;
extern int g_zUtil_ZRDR_GrowCount;
extern char g_zReader_FileExtBuf[0x100];
extern char g_zReader_FileNameBuf[0x100];
extern char g_zRdr_SplitFileNameBuf[0x100];
extern char g_zRdr_SplitExtBuf[0x100];
extern char g_zRdr_PathJoinBuf[0x100];
extern char g_zRdr_ResolvedPathBuf[0x100];
extern char g_zRdr_SplitDirBuf[0x100];
extern char g_zRdr_SplitDriveBuf[4];
extern int g_zUtil_ZRDR_WildcardReserved;
extern char g_zRdr_PathDelimStr[2];
extern char g_zUtil_ZarPathJoinFmt[0x8];
extern char *g_zUtil_ZRDR_WildcardPath;
extern int g_zUtil_ZRDR_WildcardDigits[5];
extern int g_zUtil_ZRDR_WildcardStarCount;
extern char *g_zUtil_ZRDR_WildcardStarPtrs[5];
extern zClass_NodePartial *g_Mover_LastLoadedNode;
extern int g_zRndr_GlobalStringCount;
extern char *g_zRndr_GlobalStringTable[100];

typedef int(__fastcall *zArchiveListPredicate)(
    void *,
    void *
);

zArchiveList *__cdecl zArchiveList_CreateEmpty();
int __fastcall zArchiveList_Destroy(zArchiveList *list);
void __fastcall zArchiveList_LinkNodeBetween(
    zArchiveListNode *after,
    zArchiveListNode *newNode,
    zArchiveListNode *before
);
int __fastcall zArchiveList_PushFrontPayload(
    zArchiveList *list,
    void *payload
);
int __fastcall zArchiveList_PushBackPayload(
    zArchiveList *list,
    void *payload
);
int __fastcall zArchiveList_RemovePayload(
    zArchiveList *list,
    void *payload
);
void *__fastcall zArchiveList_FreeNode(zArchiveListNode *node);
zArchiveListNode *__fastcall zArchiveList_FindNodeByPayload(
    zArchiveList *list,
    void *payload
);
void *__fastcall zArchiveList_PopFrontPayload(zArchiveList *list);
zArchiveListNode *__fastcall zUtil_ZRDR_AllocNodeWithPayload(void *payload);
int __fastcall zUtil_ZRDR_SetSearchPath(const char *pathText);
int __fastcall zUtil_ZRDR_AppendSearchPath(const char *pathText);
int __fastcall zUtil_ZRDR_FreePathList(zArchiveList *list);
zArchiveList *__fastcall zUtil_ZRDR_FreeSearchPathList(zArchiveList *list);
void __cdecl zUtil_ZRDR_FreeScratchSearchPathList();
int __cdecl zUtil_ZRDR_ShutdownWildcardPath();
int __fastcall zUtil_ZRDR_UnloadMountedArchives(int destroyCurrentToo);
int __cdecl zUtil_ZRDR_Shutdown();
void __cdecl zUtil_ZRDR_FreeNodePool();
void __cdecl zUtil_ZRDR_GrowFreePool();
void __fastcall zUtil_ZRDR_PushFreeNode(zArchiveListNode *node);
zArchiveListNode *__fastcall zUtil_ZRDR_PopFreeNode(int allowGrow);
void *__fastcall zArchiveList_FindPayloadByPredicate(
    zArchiveList *list,
    zArchiveListPredicate predicate,
    void *userData
);
void *__fastcall zArchiveList_FindPayloadByValue(
    zArchiveList *list,
    unsigned int value
);
void *__fastcall zArchiveList_FindPayloadByPredicate_Thunk(
    zArchiveList *list,
    zArchiveListPredicate predicate,
    void *userData
);
int __fastcall zUtil_ZRDR_StrCmpPredicate(
    void *str1,
    void *str2
);
int __fastcall zUtil_ZRDR_SearchPathContainsFilePredicate(
    void *searchDir,
    void *filename
);
char *__fastcall zUtil_ZRDR_ResolvePathInSearchPathList(
    zArchiveList *searchPathList,
    const char *filename
);
FILE *__fastcall zUtil_ZRDR_OpenFileResolved(
    zArchiveList *searchPathList,
    const char *filename,
    const char *mode
);
char *__fastcall zUtil_ZRDR_InitWildcardPath(char *pattern);
char *__cdecl zUtil_ZRDR_NextWildcardPath();
zArchiveList *__fastcall zUtil_ZRDR_CreateSearchPathList(const char *pathText);
int __fastcall zReader_FileExists_Wrapper(const char *path);
zReader::Node *__fastcall zReader_FindChildRecursive(
    zReader::Node *node,
    const char *searchName,
    int startIndex
);
zReader::Node *__fastcall zReader_GetNamedNode(
    zReader::Node *parentNode,
    const char *name
);
zReader::Node *__fastcall zReader_AllocateNode(
    int headerWord,
    int fieldCount
);
int __fastcall zReader_ReadString(
    void *hFile,
    zReader::Value *outString
);
int __fastcall zReader_ReadNode(
    void *hFile,
    zReader::Node *outNode
);
void __fastcall zReader_FreeNodeRecursive(zReader::Node *node);
void *__fastcall zReader_OpenFileFromMountedArchives(const char *path);
int __fastcall zArchiveList_GetCount(zArchiveList *list);
void *__fastcall zArchiveList_GetAt(
    zArchiveList *list,
    int index
);
}

namespace zUtil {
void __fastcall ZRDR_PreallocNodePool(int count);
int __fastcall ZRDR_Init(const char *pathText);
int __fastcall ZRDR_GetFileSize(FILE *fileHandle);
void __fastcall ZRDR_AddSearchPaths(
    zArchiveList *list,
    const char *pathText
);
int __fastcall SetMissionZrdrPathsAndMountZbd(int missionId);
} // namespace zUtil

namespace zArchive {
int __fastcall MountIndexArchive(
    const char *path,
    int setCurrent
);
}

#endif
