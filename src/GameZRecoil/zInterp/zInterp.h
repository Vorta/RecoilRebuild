#pragma once

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <stddef.h>
#include <stdio.h>
#include <list>

struct zArchiveList;
struct CZNodePartial;
struct zDiPartial;

/**
 * Retail polygon handlers count vertices at +0x28 and UVs at +0xa4;
 * ModelPolygonEnd stores its resolved material pointer at +0x1c4.
 * Purpose: Hold the material and indexed geometry while building a polygon.
 */
struct zInterpPolygonState {
    zModel_MaterialPartial material;
    int pointCount;
    zVec3 polygonPoints[10];
    int uvCount;
    zClipUV uvPairs[10];
    zVec3 *normalsA;
    zVec3 normalsB[10];
    zClipUV secondaryUvPairs[10];
    zModel_MaterialPartial *polygonMaterial;
    unsigned int drawFlags;
    int flagBit8;
    union {
        zTag4Partial variantTag;
        int variantTagWord;
    };
    zDiPartial *displayInstance;
};

struct zInterpFileFrame {
    FILE *file;
    long filePos;
    int hasPreparedInput;
};

struct zInterpPreparedScriptEntry {
    char path[0x78];
    long fileTime;
    long fileOffset;
};

/**
 * Retail clears the stored header as one eight-byte record at context +0x84
 * and reads the same record from the prepared-index stream.
 * Purpose: Describe the prepared script index's magic and format version.
 */
struct zInterpPreparedScriptHeader {
    int magic;
    int version;
};
RECOIL_STATIC_ASSERT(sizeof(zInterpPreparedScriptHeader) == 8);

struct zInterpMacroEntry {
    char *name;
    char *value;
};

union zInterpVarValuePtr {
    int *intPtr;
    float *floatPtr;
    char *charPtr;
};

struct zInterpVarEntry {
    char *name;
    int type;
    zInterpVarValuePtr valuePtr;
};

typedef std::list<CZNodePartial *> zInterpScrollList;
RECOIL_STATIC_ASSERT(sizeof(zInterpScrollList) == 0x0c);

struct CZInterp;

extern int g_zInterp_EnablePreparedScripts;
extern int g_zInterp_VerboseLevel;
extern char g_zInterp_LineBuffer[1024];

extern zDiPartial *g_zInterp_Object3DCommandDi;
extern zDiPartial *g_zInterp_CurrentCycleTextureDi;
extern unsigned int g_zInterp_NodeUserDataScratch;
extern char *g_zInterp_PreparedIndexFileName;

typedef void (__cdecl *zInterp_LogFn)(
    const char *fmt,
    char *args
);

struct CZInterp {
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
    zInterpMacroEntry *macroTable;
    unsigned int macroCount;
    zInterpVarEntry *varTable;
    unsigned int varCount;
    zInterp_LogFn logFn;
    char *searchPathSpec;
    char *preparedIndexFileName;
    zArchiveList *archiveSearchList;
    FILE *preparedIndexStream;
    zInterpPreparedScriptHeader preparedIndexHeader;
    int *preparedEntryCount;
    zInterpPreparedScriptEntry *preparedEntryTable;
    int hasPreparedInput;
    FILE *currentScriptFile;
    zInterpFileFrame *fileFrameStack;
    int fileFrameCount;
    zInterpPolygonState *runtimeBlob;
    void **ptrArrayHead;
    int ptrArrayCount;
    /**
     * Retail construction, erase, insertion and traversal use VC5's allocator,
     * sentinel and count layout at 0xb0..0xbb. The leading byte is allocator
     * storage, not a character read from the search path.
     * Purpose: Own the nodes whose textures scroll on every driver tick.
     */
    zInterpScrollList scrollAlwaysList;
    CZNodePartial *scrollAlwaysDriverNode;
    int includeDepth;
    int conditionalDepth;
    void *currentNode;

    static void __cdecl Logf(
        CZInterp *ctx,
        const char *fmt,
        ...
    );
    static void __cdecl ReportErrorf(
        CZInterp *ctx,
        const char *fmt,
        ...
    );
    void IncErrorCount();
    int ReportParseError(char *commandToken);
    char * FindMacroValue(
        const char *name,
        zInterpMacroEntry **outEntry
    );
    int IsMacroTrue(const char *name);
    int SetMacro(
        const char *name,
        const char *value
    );
    void ClearMacroTable();
    void ClearVarTable();
    CZInterp(
        const char *preparedIndexPath,
        const char *searchPathText
    );
    void Destroy();
    ~CZInterp();
    int EvalConditionExpr();
    char * ExpandMacroRefs(char *lineBuf);
    char * NextToken();
    int ParseBoolToken();
    float ParseFloatToken();
    int ParseIntToken();
    zInterpVarEntry * FindVarEntry(const char *name);
    void DumpVarEntry(zInterpVarEntry *entry);
    int CommandEqualsPrefix(
        const char *prefix,
        unsigned int prefixLen
    );
    int CommandEquals(const char *other);
    char * GetCurrentCommand();
    bool ValidateArgsAndNodeType(
        int expectedArgCount,
        int expectedClassType,
        CZNodePartial *node
    );
    int ReadPreparedScriptTableCount(const zInterpPreparedScriptHeader &preparedHeader, unsigned int &preparedEntryCountValue);
    int ReadPreparedScriptIndex(zInterpPreparedScriptHeader &preparedHeader, unsigned int &preparedEntryCountValue, zInterpPreparedScriptEntry *&entries);
    int LoadPreparedScriptIndex(const char *zrdrPath);
    int FindPreparedScriptIndex(const char *commandName);
    FILE * OpenPreparedScriptStream(const char *commandName);
    int RunScriptFile(const char *filePath);
    int RunStream(
        FILE *scriptFile,
        int hasPreparedInput
    );
    int RunLine(char *lineBuffer);
    int ReadLineOrPreparedTokens(
        FILE *scriptFile,
        char *lineBuffer
    );
    int TokenizeLine(const char *line);
    int HandleBuiltinCommand(char *commandToken);
    int DispatchCoreCommand(char *commandToken);
    int EchoTokens();
    void ClearFileFrameStack();
    zInterpFileFrame * PopFileFrame();
    int PushFileFrame(
        FILE *file,
        long filePos,
        int hasPreparedInput
    );
    void PrintNodeTree(
        CZNodePartial *node,
        int indent
    );
    bool HandleScrollDisable(CZNodePartial *node);
    bool RegisterScrollAlwaysNode(
        CZNodePartial *node,
        float scrollRateU,
        float scrollRateV,
        bool installDriverCallback
    );
};

struct CRecoilInterp : CZInterp {
    CRecoilInterp();

    /**
     * Original helper evidence: no standalone authored retail function; the
     * VC5 ordinary-global probe emits this inline destructor only as the
     * generated CRT teardown call to CZInterp::~CZInterp.
     * Purpose: release the process-wide interpreter during ordinary C++ shutdown.
     */
    ~CRecoilInterp() {
    }

    virtual int DispatchHook(char *commandToken);

    static int StaticInitAndRegisterAtExit();
    static CZInterp *StaticInit();
    static int RegisterAtExit();
    static void __cdecl AtExitDestructor();
};

RECOIL_STATIC_ASSERT(sizeof(CRecoilInterp) == 0xcc);

extern CRecoilInterp g_zInterp_GlobalContext;

namespace zInterp_Object3D {
int __fastcall DefaultRenderAction(CZNodePartial *node);
void __fastcall ScrollAlwaysTickAction(CZNodePartial *wrapperNode);
} // namespace zInterp_Object3D

RECOIL_STATIC_ASSERT(sizeof(zInterpFileFrame) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpFileFrame,
        file
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpFileFrame,
        filePos
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpFileFrame,
        hasPreparedInput
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInterpPreparedScriptEntry) == 0x80);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPreparedScriptEntry,
        path
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPreparedScriptEntry,
        fileTime
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPreparedScriptEntry,
        fileOffset
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(sizeof(zInterpMacroEntry) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpMacroEntry,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpMacroEntry,
        value
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(zInterpVarValuePtr) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(zInterpVarEntry) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpVarEntry,
        name
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpVarEntry,
        type
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpVarEntry,
        valuePtr
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        material
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        pointCount
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        polygonPoints
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        uvCount
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        uvPairs
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        normalsA
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        normalsB
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        secondaryUvPairs
    ) == 0x174
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        polygonMaterial
    ) == 0x1c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        drawFlags
    ) == 0x1c8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        flagBit8
    ) == 0x1cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        variantTag
    ) == 0x1d0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInterpPolygonState,
        displayInstance
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(sizeof(zInterpPolygonState) == 0x1d8);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        tokenCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        tokenReadIndex
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        lineHadError
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        errorCount
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        parseResult
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        tempAlloc
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        tokenList
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        macroTable
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        macroCount
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        varTable
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        varCount
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        logFn
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        searchPathSpec
    ) == 0x74
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        preparedIndexFileName
    ) == 0x78
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        archiveSearchList
    ) == 0x7c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        preparedIndexStream
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        preparedIndexHeader
    ) == 0x84
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        preparedEntryCount
    ) == 0x8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        preparedEntryTable
    ) == 0x90
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        hasPreparedInput
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        currentScriptFile
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        fileFrameStack
    ) == 0x9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        fileFrameCount
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        runtimeBlob
    ) == 0xa4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        ptrArrayHead
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        ptrArrayCount
    ) == 0xac
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        scrollAlwaysList
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        scrollAlwaysDriverNode
    ) == 0xbc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        includeDepth
    ) == 0xc0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        conditionalDepth
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInterp,
        currentNode
    ) == 0xc8
);
RECOIL_STATIC_ASSERT(sizeof(CZInterp) == 0xcc);
