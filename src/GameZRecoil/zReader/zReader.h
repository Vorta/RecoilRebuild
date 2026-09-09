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

Node *__fastcall Load(
    const char *path,
    const char *extraSearchPath = 0,
    int unusedStack = 0
);
int __fastcall Free(Node *loaded);
const char *__fastcall GetString(
    Node *parentNode,
    const char *name
);
int __fastcall GetFloat(
    Node *parentNode,
    const char *name,
    float *outValue
);
int __fastcall GetInt(
    Node *parentNode,
    const char *name,
    int *outValue
);
int __fastcall FindGlobalStringPrefixIndex(const char *text);
int __fastcall FileExists(const char *path);
const char *__fastcall FindFile(
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

    zIndexArchive();
    ~zIndexArchive();
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

typedef int(__fastcall *zArchiveListCompare)(
    void *,
    void *
);

zArchiveList *__cdecl zArchiveListNew();
int __fastcall zArchiveListFree(zArchiveList *list);
void __fastcall zArchiveListLink(
    zArchiveListNode *after,
    zArchiveListNode *newNode,
    zArchiveListNode *before
);
int __fastcall zArchiveListAddHead(
    zArchiveList *list,
    void *payload
);
int __fastcall zArchiveListAddTail(
    zArchiveList *list,
    void *payload
);
int __fastcall zArchiveListRemove(
    zArchiveList *list,
    void *payload
);
void *__fastcall zArchiveListFreeNode(zArchiveListNode *node);
zArchiveListNode *__fastcall zArchiveListFindNode(
    zArchiveList *list,
    void *payload
);
void *__fastcall zArchiveListRemoveHead(zArchiveList *list);
zArchiveListNode *__fastcall zArchiveListAllocNode(void *payload);
int __fastcall zRdrSetPath(const char *pathText);
int __fastcall zRdrAddPath(const char *pathText);
int __fastcall zRdrFreePathList(zArchiveList *list);
zArchiveList *__fastcall zRdrFreeSearchPathList(zArchiveList *list);
void __cdecl zRdrFreeScratchSearchPathList();
int __cdecl zRdrShutdownWildcardPath();
void __fastcall zRdrUnmount(int destroyCurrentToo);
int __cdecl zRdrExit();
void __cdecl zRdrFreeNodePool();
void __cdecl zRdrGrowNodePool();
void __fastcall zRdrFreeNode(zArchiveListNode *node);
zArchiveListNode *__fastcall zUtil_ZRDR_AllocNode(int allowGrow);
void *__fastcall zArchiveListFindCompare(
    zArchiveList *list,
    zArchiveListCompare predicate,
    void *userData
);
void *__fastcall zArchiveListFindKey(
    zArchiveList *list,
    unsigned int value
);
void *__fastcall zArchiveListFind(
    zArchiveList *list,
    zArchiveListCompare predicate,
    void *userData
);
int __fastcall zRdrStrCmpPredicate(
    void *str1,
    void *str2
);
int __fastcall zRdrSearchPathContainsFilePredicate(
    void *searchDir,
    void *filename
);
char *__fastcall zRdrResolvePathInSearchPathList(
    zArchiveList *searchPathList,
    const char *filename
);
FILE *__fastcall zRdrOpenFileResolved(
    zArchiveList *searchPathList,
    const char *filename,
    const char *mode
);
char *__fastcall zRdrInitWildcardPath(char *pattern);
char *__cdecl zRdrNextWildcardPath();
zArchiveList *__fastcall zRdrCreateSearchPathList(const char *pathText);
int __fastcall zReaderFileExistsWrapper(const char *path);
zReader::Node *__fastcall zRdrFindNode(
    zReader::Node *node,
    const char *searchName,
    int startIndex
);
zReader::Node *__fastcall zRdrGetNode(
    zReader::Node *parentNode,
    const char *name
);
zReader::Node *__fastcall zRdrAllocNode(
    int headerWord,
    int fieldCount
);
int __fastcall zReaderReadString(
    void *hFile,
    zReader::Value *outString
);
int __fastcall zRdrRead(
    void *hFile,
    zReader::Node *outNode
);
void __fastcall zRdrFreeContents(zReader::Node *node);
void *__fastcall zRdrOpenFile(const char *path);
int __fastcall zArchiveListCount(zArchiveList *list);
void *__fastcall zArchiveListGet(
    zArchiveList *list,
    int index
);
}

namespace zUtil {
void __fastcall zRdrInitNodePool(int count);
int __fastcall zRdrInit(const char *pathText);
int __fastcall zRdrGetFileSize(FILE *fileHandle);
void __fastcall zRdrAddSearchPaths(
    zArchiveList *list,
    const char *pathText
);
int __fastcall SetMissionZrdrPathsAndMountZbd(int missionId);
} // namespace zUtil

namespace zArchive {
int __fastcall Mount(
    const char *path,
    int setCurrent
);
}

#endif
