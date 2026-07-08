#pragma once

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <stddef.h>
#include <stdio.h>

struct zArchiveList;
struct zClass_NodePartial;
struct zDiPartial;

struct zInterp_RuntimeBlob {
    zModel_MaterialPartial material;
    zModel_MaterialPartial *polygonMaterial;
    zVec3 polygonPoints[10];
    int pointCount;
    zClipUV uvPairs[10];
    zVec3 *normalsA;
    zVec3 normalsB[10];
    zClipUV secondaryUvPairs[10];
    int uvCount;
    unsigned int drawFlags;
    int flagBit8;
    union {
        zTag4Partial variantTag;
        int variantTagWord;
    };
    zDiPartial *displayInstance;
};

struct zInterp_FileFrame {
    FILE *file;
    long filePos;
    int hasPreparedInput;
};

struct zInterp_PreparedScriptEntry {
    char path[0x78];
    long fileTime;
    long fileOffset;
};

struct zInterp_MacroEntry {
    char *name;
    char *value;
};

union zInterp_VarValuePtr {
    int *intPtr;
    float *floatPtr;
    char *charPtr;
};

struct zInterp_VarEntry {
    char *name;
    int type;
    zInterp_VarValuePtr valuePtr;
};

struct zInterp_LinkNode {
    zInterp_LinkNode *next;
    zInterp_LinkNode *prev;
    void *payload;
};

struct zInterp_Context;

extern int g_zInterp_EnablePreparedScripts;
extern int g_zInterp_VerboseLevel;
extern char g_zInterp_LineBuffer[1024];
extern char g_zInterp_AssignToken_Equal;
extern int g_zInterp_Object3DCommandIntScratch;
extern zDiPartial *g_zInterp_CurrentCycleTextureDi;
extern unsigned int g_zInterp_NodeUserDataScratch;
extern char *g_zInterp_PreparedIndexFileName;

typedef void(*zInterp_LogFn)(
    const char *fmt,
    char *args
);

struct zInterp_Context {
    virtual int DispatchHook(char *commandToken);
    virtual int PostDispatchHook(char *commandToken);
    virtual int DeferredDispatchHook(char *commandToken);

    unsigned int unknown_04;
    unsigned int tokenCount;
    int tokenReadIndex;
    int lineHadError;
    int errorCount;
    int parseResult;
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
    FILE *currentScriptFile;
    zInterp_FileFrame *fileFrameStack;
    int fileFrameCount;
    zInterp_RuntimeBlob *runtimeBlob;
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

    static void Logf(
        zInterp_Context *ctx,
        const char *fmt,
        ...
    );
    static void ReportErrorf(
        zInterp_Context *ctx,
        const char *fmt,
        ...
    );
    void IncErrorCount();
    int ReportParseError(char *commandToken);
    char * FindMacroValue(
        const char *name,
        zInterp_MacroEntry **outEntry
    );
    int IsMacroTrue(const char *name);
    int SetMacro(
        const char *name,
        const char *value
    );
    void ClearMacroTable();
    void ClearVarTable();
    zInterp_Context * Constructor(
        const char *searchPathText,
        const char *preparedIndexPath
    );
    void Destroy();
    void Destructor();
    int EvalConditionExpr();
    char * ExpandMacroRefs(char *lineBuf);
    char * NextToken();
    int ParseBoolToken();
    float ParseFloatToken();
    int ParseIntToken();
    zInterp_VarEntry * FindVarEntry(const char *name);
    void DumpVarEntry(zInterp_VarEntry *entry);
    int CommandEqualsPrefix(
        const char *prefix,
        unsigned int prefixLen
    );
    int CommandEquals(const char *other);
    char * GetCurrentCommand();
    int ValidateArgsAndNodeType(
        int expectedArgCount,
        int expectedClassType,
        zClass_NodePartial *node
    );
    int LoadPreparedScriptIndex(const char *zrdrPath);
    FILE * OpenPreparedScriptStream(const char *commandName);
    int RunScriptFile(const char *filePath);
    int RunString(
        FILE *scriptFile,
        int hasPreparedInput
    );
    int RunStream(char *lineBuffer);
    int ReadLineOrPreparedTokens(
        FILE *scriptFile,
        char *lineBuffer
    );
    int TokenizeLine(const char *line);
    int HandleBuiltinCommand(char *commandToken);
    int DispatchCoreCommand(char *commandToken);
    int EchoTokens();
    void ClearFileFrameStack();
    zInterp_FileFrame * PopFileFrame();
    int PushFileFrame(
        FILE *file,
        long filePos,
        int hasPreparedInput
    );
    void PrintNodeTree(
        zClass_NodePartial *node,
        int indent
    );
    static int __stdcall DefaultDispatchHook(zClass_NodePartial *node);
    int RegisterScrollAlwaysNode(
        zClass_NodePartial *node,
        float textureWorldPerMeter,
        int textureWorldAxis,
        int installDriverCallback
    );
};

struct zInterp_GlobalContext : zInterp_Context {
    zInterp_GlobalContext();
    virtual int DispatchHook(char *commandToken);

    static int StaticInitAndRegisterAtExit();
    static zInterp_Context *StaticInit();
    static int RegisterAtExit();
    static void AtExitDestructor();
};

RECOIL_STATIC_ASSERT(sizeof(zInterp_GlobalContext) == 0xcc);

union zInterp_GlobalContextStorage {
    unsigned long align;
    unsigned char bytes[sizeof(zInterp_GlobalContext)];
};
RECOIL_STATIC_ASSERT(sizeof(zInterp_GlobalContextStorage) == 0xcc);

extern zInterp_GlobalContextStorage g_zInterp_GlobalContext;
#define g_zInterp_GlobalContext \
    (*(zInterp_GlobalContext *)&g_zInterp_GlobalContext)

namespace zInterp_Object3D {
int __fastcall DefaultRenderAction(zClass_NodePartial *node);
void __fastcall ScrollAlwaysTickAction(zClass_NodePartial *wrapperNode);
} // namespace zInterp_Object3D

RECOIL_STATIC_ASSERT(sizeof(zInterp_FileFrame) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_FileFrame,
        file
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_FileFrame,
        filePos
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_FileFrame,
        hasPreparedInput
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_PreparedScriptEntry) == 0x80);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_PreparedScriptEntry,
        path
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_PreparedScriptEntry,
        fileTime
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_PreparedScriptEntry,
        fileOffset
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_MacroEntry) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_MacroEntry,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_MacroEntry,
        value
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_VarValuePtr) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zInterp_VarEntry) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_VarEntry,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_VarEntry,
        type
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_VarEntry,
        valuePtr
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_LinkNode) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_LinkNode,
        next
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_LinkNode,
        prev
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_LinkNode,
        payload
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        material
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        polygonMaterial
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        polygonPoints
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        pointCount
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        uvPairs
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        normalsA
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        normalsB
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        secondaryUvPairs
    ) == 0x174
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        uvCount
    ) == 0x1c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        drawFlags
    ) == 0x1c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        flagBit8
    ) == 0x1cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        variantTag
    ) == 0x1d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_RuntimeBlob,
        displayInstance
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_RuntimeBlob) == 0x1d8);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        tokenCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        tokenReadIndex
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        lineHadError
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        errorCount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        parseResult
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        tempAlloc
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        tokenList
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        macroTable
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        macroCount
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        varTable
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        varCount
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        logFn
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        searchPathSpec
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedIndexFileName
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        archiveSearchList
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedIndexStream
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedIndexMagic
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedIndexVersion
    ) == 0x88
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedEntryCount
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        preparedEntryTable
    ) == 0x90
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        hasPreparedInput
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        currentScriptFile
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        fileFrameStack
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        fileFrameCount
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        runtimeBlob
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        ptrArrayHead
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        ptrArrayCount
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        searchPathLeadChar
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        scrollAlwaysListHead
    ) == 0xb4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        scrollAlwaysListCount
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        scrollAlwaysDriverNode
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        includeDepth
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        conditionalDepth
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterp_Context,
        currentNode
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(sizeof(zInterp_Context) == 0xcc);
