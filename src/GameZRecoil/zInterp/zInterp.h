#pragma once

#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <stddef.h>
#include <stdio.h>

struct zArchiveList;
struct zClass_NodePartial;

struct zInterp_FileFrame
{
    FILE *file;
    long filePos;
    int hasPreparedInput;
};

struct zInterp_PreparedScriptEntry
{
    char path[0x78];
    long fileTime;
    long fileOffset;
};

struct zInterp_MacroEntry
{
    char *name;
    char *value;
};

union zInterp_VarValuePtr
{
    int *intPtr;
    float *floatPtr;
    char *charPtr;
};

struct zInterp_VarEntry
{
    char *name;
    int type;
    zInterp_VarValuePtr valuePtr;
};

struct zInterp_LinkNode
{
    zInterp_LinkNode *next;
    zInterp_LinkNode *prev;
    void *payload;
};

extern const int g_zInterp_Context_VTableMarker;

typedef void(RECOIL_CDECL *zInterp_LogFn)(const char *fmt, char *args);

struct zInterp_Context
{
    const void *vftable;
    unsigned int unknown_04;
    unsigned int tokenCount;
    int tokenReadIndex;
    int lineHadError;
    int errorCount;
    unsigned int unknown_18;
    char *tempAlloc;
    char *tokenList[16];
    zInterp_MacroEntry *macroTable;
    unsigned int macroCount;
    zInterp_VarEntry *varTable;
    unsigned int varCount;
    zInterp_LogFn logFn;
    char *searchPathSpec;
    char *preparedIndexFileName;
    zArchiveList *archiveSearchList;
    FILE *preparedIndexStream;
    int preparedIndexMagic;
    int preparedIndexVersion;
    int *preparedEntryCount;
    zInterp_PreparedScriptEntry *preparedEntryTable;
    int hasPreparedInput;
    unsigned int unknown_98;
    zInterp_FileFrame *fileFrameStack;
    int fileFrameCount;
    void *runtimeBlob;
    void **ptrArrayHead;
    int ptrArrayCount;
    char searchPathLeadChar;
    unsigned char unknown_b1[3];
    zInterp_LinkNode *scrollAlwaysListHead;
    int scrollAlwaysListCount;
    zClass_NodePartial *scrollAlwaysDriverNode;
    int includeDepth;
    int conditionalDepth;
    void *currentNode;

    RECOIL_NOINLINE static void RECOIL_CDECL Logf(zInterp_Context *ctx, const char *fmt, ...);
    RECOIL_NOINLINE static void RECOIL_CDECL ReportErrorf(zInterp_Context *ctx, const char *fmt,
                                                          ...);
    RECOIL_NOINLINE void RECOIL_THISCALL IncErrorCount();
    RECOIL_NOINLINE char *RECOIL_THISCALL FindMacroValue(const char *name,
                                                         zInterp_MacroEntry **outEntry);
    RECOIL_NOINLINE int RECOIL_THISCALL IsMacroTrue(const char *name);
    RECOIL_NOINLINE int RECOIL_THISCALL SetMacro(const char *name, const char *value);
    RECOIL_NOINLINE void RECOIL_THISCALL ClearMacroTable();
    RECOIL_NOINLINE void RECOIL_THISCALL ClearVarTable();
    RECOIL_NOINLINE zInterp_Context *RECOIL_THISCALL Constructor(const char *searchPathText,
                                                                const char *preparedIndexPath);
    RECOIL_NOINLINE void RECOIL_THISCALL Destroy();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE int RECOIL_THISCALL EvalConditionExpr();
    RECOIL_NOINLINE char *RECOIL_THISCALL ExpandMacroRefs(char *lineBuf);
    RECOIL_NOINLINE char *RECOIL_THISCALL NextToken();
    RECOIL_NOINLINE int RECOIL_THISCALL ParseBoolToken();
    RECOIL_NOINLINE float RECOIL_THISCALL ParseFloatToken();
    RECOIL_NOINLINE int RECOIL_THISCALL ParseIntToken();
    RECOIL_NOINLINE zInterp_VarEntry *RECOIL_THISCALL FindVarEntry(const char *name);
    RECOIL_NOINLINE void RECOIL_THISCALL DumpVarEntry(zInterp_VarEntry *entry);
    RECOIL_NOINLINE int RECOIL_THISCALL CommandEqualsPrefix(const char *prefix,
                                                            unsigned int prefixLen);
    RECOIL_NOINLINE int RECOIL_THISCALL CommandEquals(const char *other);
    RECOIL_NOINLINE char *RECOIL_THISCALL GetCurrentCommand();
    RECOIL_NOINLINE int RECOIL_THISCALL ValidateArgsAndNodeType(
        int expectedArgCount, int expectedClassType, zClass_NodePartial *node);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadPreparedScriptIndex(const char *zrdrPath);
    RECOIL_NOINLINE FILE *RECOIL_THISCALL OpenPreparedScriptStream(const char *commandName);
    RECOIL_NOINLINE int RECOIL_THISCALL ReadLineOrPreparedTokens(FILE *scriptFile,
                                                                 char *lineBuffer);
    RECOIL_NOINLINE int RECOIL_THISCALL TokenizeLine(const char *line);
    RECOIL_NOINLINE int RECOIL_THISCALL EchoTokens();
    RECOIL_NOINLINE void RECOIL_THISCALL ClearFileFrameStack();
    RECOIL_NOINLINE zInterp_FileFrame *RECOIL_THISCALL PopFileFrame();
    RECOIL_NOINLINE int RECOIL_THISCALL PushFileFrame(FILE *file, long filePos,
                                                      int hasPreparedInput);
    RECOIL_NOINLINE void RECOIL_THISCALL PrintNodeTree(zClass_NodePartial *node, int indent);
    RECOIL_NOINLINE static int RECOIL_STDCALL DefaultDispatchHook(zClass_NodePartial *node);
    RECOIL_NOINLINE int RECOIL_THISCALL RegisterScrollAlwaysNode(
        zClass_NodePartial *node, float textureWorldPerMeter, int textureWorldAxis,
        int installDriverCallback);
};

namespace zInterp_Object3D
{
RECOIL_NOINLINE int RECOIL_FASTCALL DefaultRenderAction(zClass_NodePartial *node);
RECOIL_NOINLINE void RECOIL_FASTCALL ScrollAlwaysTickAction(zClass_NodePartial *wrapperNode);
} // namespace zInterp_Object3D

RECOIL_STATIC_ASSERT(sizeof(zInterp_FileFrame) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_FileFrame, file) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInterp_FileFrame, filePos) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInterp_FileFrame, hasPreparedInput) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zInterp_PreparedScriptEntry) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zInterp_PreparedScriptEntry, path) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInterp_PreparedScriptEntry, fileTime) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zInterp_PreparedScriptEntry, fileOffset) == 0x7c);
RECOIL_STATIC_ASSERT(sizeof(zInterp_MacroEntry) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInterp_MacroEntry, name) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInterp_MacroEntry, value) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zInterp_VarValuePtr) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zInterp_VarEntry) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_VarEntry, name) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInterp_VarEntry, type) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInterp_VarEntry, valuePtr) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zInterp_LinkNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_LinkNode, next) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInterp_LinkNode, prev) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInterp_LinkNode, payload) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, tokenCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, tokenReadIndex) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, lineHadError) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, errorCount) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, tempAlloc) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, tokenList) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, macroTable) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, macroCount) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, varTable) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, varCount) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, logFn) == 0x70);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, searchPathSpec) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedIndexFileName) == 0x78);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, archiveSearchList) == 0x7c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedIndexStream) == 0x80);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedIndexMagic) == 0x84);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedIndexVersion) == 0x88);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedEntryCount) == 0x8c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, preparedEntryTable) == 0x90);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, hasPreparedInput) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, fileFrameStack) == 0x9c);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, fileFrameCount) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, runtimeBlob) == 0xa4);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, ptrArrayHead) == 0xa8);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, ptrArrayCount) == 0xac);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, searchPathLeadChar) == 0xb0);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, scrollAlwaysListHead) == 0xb4);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, scrollAlwaysListCount) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, scrollAlwaysDriverNode) == 0xbc);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, includeDepth) == 0xc0);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, conditionalDepth) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(zInterp_Context, currentNode) == 0xc8);
RECOIL_STATIC_ASSERT(sizeof(zInterp_Context) == 0xcc);
