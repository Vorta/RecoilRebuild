#include "GameZRecoil/zInterp/zInterp.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <io.h>
#include <windows.h>

namespace
{
void WriteU32(FILE *file, unsigned int value)
{
    std::fwrite(&value, sizeof(value), 1, file);
}

const char *BaseName(const char *path)
{
    const char *slash = std::strrchr(path, '\\');
    return slash != nullptr ? slash + 1 : path;
}

bool WritePreparedIndex(const char *path, const zInterp_PreparedScriptEntry *entries,
                        unsigned int entryCount)
{
    FILE *const file = std::fopen(path, "wb");
    if (file == nullptr) {
        return false;
    }

    WriteU32(file, 0x08971119);
    WriteU32(file, 7);
    WriteU32(file, entryCount);
    if (entryCount != 0) {
        std::fwrite(entries, sizeof(zInterp_PreparedScriptEntry), entryCount, file);
    }
    std::fclose(file);
    return true;
}

void SetCommandTokens(zInterp_Context *context, char **tokens, unsigned int tokenCount)
{
    context->tokenCount = tokenCount;
    context->tokenReadIndex = 1;
    for (unsigned int i = 0; i < tokenCount; ++i) {
        context->tokenList[i] = tokens[i];
    }
}

const char *g_logFmt = nullptr;
int g_logValue = 0;
const char *g_logNameArg = nullptr;
int g_logSecondInt = 0;
double g_logSecondDouble = 0.0;
const char *g_logThirdNameArg = nullptr;
int g_logThirdInt = 0;
int g_logFourthInt = 0;
int g_logCallCount = 0;

void RECOIL_CDECL TestLogFn(const char *fmt, char *args)
{
    int *const argWords = reinterpret_cast<int *>(args);
    g_logFmt = fmt;
    g_logValue = argWords[0];
    g_logNameArg = reinterpret_cast<const char *>(argWords[0]);
    g_logSecondInt = argWords[1];
    g_logSecondDouble = *reinterpret_cast<double *>(&argWords[1]);
    g_logThirdNameArg = reinterpret_cast<const char *>(argWords[2]);
    g_logThirdInt = argWords[2];
    g_logFourthInt = argWords[3];
    ++g_logCallCount;
}

struct TestZrdrFreePoolScope
{
    zArchiveList *oldFreePool;
    int oldTotalAllocated;
    int oldFreeCount;
    int oldGrowCount;
    bool createdPool;

    TestZrdrFreePoolScope()
        : oldFreePool(g_zUtil_ZRDR_FreePool),
          oldTotalAllocated(g_zUtil_ZRDR_TotalAllocated),
          oldFreeCount(g_zUtil_ZRDR_FreeCount),
          oldGrowCount(g_zUtil_ZRDR_GrowCount),
          createdPool(false)
    {
        if (g_zUtil_ZRDR_FreePool == nullptr) {
            g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
            createdPool = true;
        }
    }

    ~TestZrdrFreePoolScope()
    {
        if (createdPool) {
            zUtil_ZRDR_FreeNodePool();
            g_zUtil_ZRDR_FreePool = oldFreePool;
            g_zUtil_ZRDR_TotalAllocated = oldTotalAllocated;
            g_zUtil_ZRDR_FreeCount = oldFreeCount;
            g_zUtil_ZRDR_GrowCount = oldGrowCount;
        }
    }
};
} // namespace

extern "C" int zinterp_context_logf_smoke()
{
    zInterp_Context context = {};
    g_logFmt = nullptr;
    g_logValue = 0;
    g_logCallCount = 0;

    zInterp_Context::Logf(&context, "ignored %d", 11);
    const bool nullHookOk = g_logCallCount == 0;

    context.logFn = TestLogFn;
    zInterp_Context::Logf(&context, "value %d", 42);
    return nullHookOk && g_logCallCount == 1 && g_logFmt != nullptr &&
                   std::strcmp(g_logFmt, "value %d") == 0 && g_logValue == 42
               ? 0
               : 1;
}

extern "C" int zinterp_context_report_errorf_smoke()
{
    zInterp_Context context = {};
    g_logFmt = nullptr;
    g_logValue = 0;
    g_logCallCount = 0;

    zInterp_Context::ReportErrorf(&context, "quiet %d", 7);
    const bool nullHookOk = context.lineHadError == 1 && g_logCallCount == 0;

    context.lineHadError = 0;
    context.logFn = TestLogFn;
    zInterp_Context::ReportErrorf(&context, "error %d", 55);
    return nullHookOk && context.lineHadError == 1 && g_logCallCount == 1 &&
                   g_logFmt != nullptr && std::strcmp(g_logFmt, "error %d") == 0 &&
                   g_logValue == 55
               ? 0
               : 1;
}

extern "C" int zinterp_context_inc_error_count_smoke()
{
    zInterp_Context context = {};
    context.errorCount = 4;
    context.IncErrorCount();
    context.IncErrorCount();
    return context.errorCount == 6 ? 0 : 1;
}

extern "C" int zinterp_context_find_macro_value_smoke()
{
    char alphaName[] = "alpha";
    char alphaValue[] = "1";
    char betaName[] = "beta";
    char betaValue[] = "enabled";
    zInterp_MacroEntry entries[2] = {
        {alphaName, alphaValue},
        {betaName, betaValue},
    };
    zInterp_Context context = {};
    context.macroTable = entries;
    context.macroCount = 2;

    zInterp_MacroEntry *foundEntry = nullptr;
    char *const foundValue = context.FindMacroValue("beta", &foundEntry);
    const bool foundOk =
        foundValue == entries[1].value && foundEntry == &entries[1];

    zInterp_MacroEntry *unchangedEntry = &entries[0];
    char *const missingValue = context.FindMacroValue("missing", &unchangedEntry);
    const bool missingOk = missingValue == nullptr && unchangedEntry == &entries[0];

    zInterp_Context emptyContext = {};
    zInterp_MacroEntry *emptyEntry = &entries[1];
    char *const emptyValue = emptyContext.FindMacroValue("alpha", &emptyEntry);
    const bool emptyOk = emptyValue == nullptr && emptyEntry == &entries[1];

    return foundOk && missingOk && emptyOk ? 0 : 1;
}

extern "C" int zinterp_context_is_macro_true_smoke()
{
    char trueName[] = "enabled";
    char trueValue[] = "TRUE";
    char falseName[] = "disabled";
    char falseValue[] = "FALSE";
    char lowerName[] = "lower";
    char lowerValue[] = "true";
    zInterp_MacroEntry entries[3] = {
        {trueName, trueValue},
        {falseName, falseValue},
        {lowerName, lowerValue},
    };
    zInterp_Context context = {};
    context.macroTable = entries;
    context.macroCount = 3;

    const bool ok = context.IsMacroTrue("enabled") == 1 &&
                    context.IsMacroTrue("disabled") == 0 &&
                    context.IsMacroTrue("lower") == 0 &&
                    context.IsMacroTrue("missing") == 0;
    return ok ? 0 : 1;
}

extern "C" int zinterp_context_set_macro_smoke()
{
    zInterp_Context context = {};
    const bool nullOk = context.SetMacro(nullptr, "value") == 0 &&
                        context.SetMacro("name", nullptr) == 0 && context.macroCount == 0;

    const int addResult = context.SetMacro("alpha", "one");
    const bool addOk = addResult == 1 && context.macroCount == 1 &&
                       context.macroTable != nullptr &&
                       std::strcmp(context.macroTable[0].name, "alpha") == 0 &&
                       std::strcmp(context.macroTable[0].value, "one") == 0;

    char *const oldName = context.macroTable[0].name;
    const int updateResult = context.SetMacro("alpha", "two");
    const bool updateOk = updateResult == 1 && context.macroCount == 1 &&
                          context.macroTable[0].name == oldName &&
                          std::strcmp(context.macroTable[0].value, "two") == 0;

    const int secondResult = context.SetMacro("beta", "enabled");
    const bool secondOk = secondResult == 1 && context.macroCount == 2 &&
                          std::strcmp(context.macroTable[1].name, "beta") == 0 &&
                          std::strcmp(context.macroTable[1].value, "enabled") == 0;

    for (unsigned int i = 0; i < context.macroCount; ++i) {
        std::free(context.macroTable[i].name);
        std::free(context.macroTable[i].value);
    }
    std::free(context.macroTable);
    return nullOk && addOk && updateOk && secondOk ? 0 : 1;
}

extern "C" int zinterp_context_clear_tables_smoke()
{
    zInterp_MacroEntry *const macroEntries =
        static_cast<zInterp_MacroEntry *>(std::malloc(2 * sizeof(zInterp_MacroEntry)));
    if (macroEntries == nullptr) {
        return 1;
    }
    macroEntries[0].name = _strdup("A");
    macroEntries[0].value = _strdup("TRUE");
    macroEntries[1].name = _strdup("B");
    macroEntries[1].value = _strdup("FALSE");

    zInterp_Context macroContext = {};
    macroContext.macroTable = macroEntries;
    macroContext.macroCount = 2;
    macroContext.ClearMacroTable();
    const bool macroOk = macroContext.macroTable == nullptr && macroContext.macroCount == 0;

    int value = 5;
    zInterp_VarEntry *const varEntries =
        static_cast<zInterp_VarEntry *>(std::malloc(2 * sizeof(zInterp_VarEntry)));
    if (varEntries == nullptr) {
        return 2;
    }
    varEntries[0].name = _strdup("health");
    varEntries[0].type = 0;
    varEntries[0].valuePtr.intPtr = &value;
    varEntries[1].name = _strdup("speed");
    varEntries[1].type = 0;
    varEntries[1].valuePtr.intPtr = &value;

    zInterp_Context varContext = {};
    varContext.varTable = varEntries;
    varContext.varCount = 2;
    varContext.ClearVarTable();
    const bool varOk = varContext.varTable == nullptr && varContext.varCount == 0 && value == 5;

    zInterp_FileFrame *const frames =
        static_cast<zInterp_FileFrame *>(std::malloc(2 * sizeof(zInterp_FileFrame)));
    if (frames == nullptr) {
        return 3;
    }

    zInterp_Context fileContext = {};
    fileContext.fileFrameStack = frames;
    fileContext.fileFrameCount = 2;
    fileContext.ClearFileFrameStack();
    const bool fileOk =
        fileContext.fileFrameStack == nullptr && fileContext.fileFrameCount == 0;

    zInterp_Context empty = {};
    empty.ClearMacroTable();
    empty.ClearVarTable();
    empty.ClearFileFrameStack();
    const bool emptyOk = empty.macroTable == nullptr && empty.macroCount == 0 &&
                         empty.varTable == nullptr && empty.varCount == 0 &&
                         empty.fileFrameStack == nullptr && empty.fileFrameCount == 0;

    return macroOk && varOk && fileOk && emptyOk ? 0 : 4;
}

extern "C" int zinterp_context_destroy_smoke()
{
    TestZrdrFreePoolScope freePoolScope;

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zid", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zje", 0, tempPathB) == 0) {
        return 1;
    }

    char searchText[MAX_PATH * 3] = {};
    std::strcpy(searchText, tempPathA);
    std::strcat(searchText, ";");
    std::strcat(searchText, tempPathB);

    zInterp_Context context = {};
    context.macroTable =
        static_cast<zInterp_MacroEntry *>(std::malloc(sizeof(zInterp_MacroEntry)));
    context.macroTable[0].name = _strdup("MAC");
    context.macroTable[0].value = _strdup("TRUE");
    context.macroCount = 1;

    int varValue = 9;
    context.varTable =
        static_cast<zInterp_VarEntry *>(std::malloc(sizeof(zInterp_VarEntry)));
    context.varTable[0].name = _strdup("var");
    context.varTable[0].type = 0;
    context.varTable[0].valuePtr.intPtr = &varValue;
    context.varCount = 1;

    context.fileFrameStack =
        static_cast<zInterp_FileFrame *>(std::malloc(sizeof(zInterp_FileFrame)));
    context.fileFrameCount = 1;
    context.archiveSearchList = zUtil_ZRDR_CreateSearchPathList(searchText);

    zInterp_LinkNode sentinel = {};
    zInterp_LinkNode *const nodeA = new zInterp_LinkNode;
    zInterp_LinkNode *const nodeB = new zInterp_LinkNode;
    sentinel.next = nodeA;
    sentinel.prev = nodeB;
    nodeA->next = nodeB;
    nodeA->prev = &sentinel;
    nodeB->next = &sentinel;
    nodeB->prev = nodeA;
    context.scrollAlwaysListHead = &sentinel;
    context.scrollAlwaysListCount = 2;
    context.scrollAlwaysDriverNode = (zClass_NodePartial *)nodeA;
    context.ptrArrayHead = static_cast<void **>(std::malloc(2 * sizeof(void *)));
    context.ptrArrayCount = 2;
    context.includeDepth = 3;
    context.conditionalDepth = 4;

    if (context.archiveSearchList == nullptr || context.ptrArrayHead == nullptr) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    context.Destroy();

    const bool tablesOk = context.macroTable == nullptr && context.macroCount == 0 &&
                          context.varTable == nullptr && context.varCount == 0 &&
                          context.fileFrameStack == nullptr && context.fileFrameCount == 0 &&
                          context.archiveSearchList == nullptr;
    const bool listOk = sentinel.next == &sentinel && sentinel.prev == &sentinel &&
                        context.scrollAlwaysListCount == 0 &&
                        context.scrollAlwaysDriverNode == nullptr;
    const bool tailOk = context.ptrArrayHead == nullptr && context.ptrArrayCount == 0 &&
                        context.includeDepth == 0 && context.conditionalDepth == 4;

    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return tablesOk && listOk && tailOk ? 0 : 3;
}

extern "C" int zinterp_context_destructor_smoke()
{
    TestZrdrFreePoolScope freePoolScope;

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zkd", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zle", 0, tempPathB) == 0) {
        return 1;
    }

    char searchText[MAX_PATH * 3] = {};
    std::strcpy(searchText, tempPathA);
    std::strcat(searchText, ";");
    std::strcat(searchText, tempPathB);

    zInterp_Context context = {};
    context.vftable = nullptr;
    context.tempAlloc = _strdup("temp");
    context.runtimeBlob =
        static_cast<zInterp_RuntimeBlob *>(std::malloc(sizeof(zInterp_RuntimeBlob)));
    context.preparedEntryCount = static_cast<int *>(std::malloc(sizeof(int)));
    context.preparedEntryTable = static_cast<zInterp_PreparedScriptEntry *>(
        std::malloc(sizeof(zInterp_PreparedScriptEntry)));
    context.preparedIndexFileName = _strdup("prepared.idx");
    context.searchPathSpec = _strdup(searchText);
    context.archiveSearchList = zUtil_ZRDR_CreateSearchPathList(searchText);
    context.ptrArrayHead = static_cast<void **>(std::malloc(sizeof(void *)));
    context.ptrArrayCount = 1;
    context.includeDepth = 5;

    zInterp_LinkNode *const sentinel = new zInterp_LinkNode;
    zInterp_LinkNode *const node = new zInterp_LinkNode;
    sentinel->next = node;
    sentinel->prev = node;
    node->next = sentinel;
    node->prev = sentinel;
    context.scrollAlwaysListHead = sentinel;
    context.scrollAlwaysListCount = 1;
    context.scrollAlwaysDriverNode = (zClass_NodePartial *)node;

    if (context.tempAlloc == nullptr || context.runtimeBlob == nullptr ||
        context.preparedEntryCount == nullptr || context.preparedEntryTable == nullptr ||
        context.preparedIndexFileName == nullptr || context.searchPathSpec == nullptr ||
        context.archiveSearchList == nullptr || context.ptrArrayHead == nullptr) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    context.Destructor();

    const bool ok = context.vftable == &g_zInterp_Context_VTableMarker &&
                    context.scrollAlwaysListHead == nullptr &&
                    context.scrollAlwaysListCount == 0 &&
                    context.ptrArrayHead == nullptr && context.ptrArrayCount == 0 &&
                    context.includeDepth == 0;

    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return ok ? 0 : 3;
}

extern "C" int zinterp_context_constructor_smoke()
{
    char searchPath[] = "support\\scripts";
    char preparedIndex[] = "scripts.idx";

    zInterp_Context context = {};
    zInterp_Context *const returned = context.Constructor(searchPath, preparedIndex);

    const unsigned char *const runtime =
        static_cast<const unsigned char *>(static_cast<const void *>(context.runtimeBlob));
    bool runtimeZeroed = runtime != nullptr;
    for (int index = 0; runtimeZeroed && index < 0x1d8; ++index) {
        if (runtime[index] != 0) {
            runtimeZeroed = false;
        }
    }

    const bool ok = returned == &context &&
                    context.vftable == &g_zInterp_Context_VTableMarker &&
                    context.searchPathLeadChar == 's' &&
                    context.scrollAlwaysListHead != nullptr &&
                    context.scrollAlwaysListHead->next == context.scrollAlwaysListHead &&
                    context.scrollAlwaysListHead->prev == context.scrollAlwaysListHead &&
                    context.scrollAlwaysListCount == 0 && context.includeDepth == 0 &&
                    context.runtimeBlob != nullptr && runtimeZeroed &&
                    context.currentNode == nullptr && context.conditionalDepth == 0 &&
                    context.tempAlloc == nullptr && context.hasPreparedInput == 0 &&
                    context.tokenCount == 0 && context.logFn == nullptr &&
                    context.scrollAlwaysDriverNode == nullptr &&
                    context.searchPathSpec != nullptr &&
                    std::strcmp(context.searchPathSpec, searchPath) == 0 &&
                    context.preparedIndexFileName != nullptr &&
                    std::strcmp(context.preparedIndexFileName, preparedIndex) == 0 &&
                    context.archiveSearchList == nullptr && context.fileFrameStack == nullptr &&
                    context.fileFrameCount == 0 && context.preparedIndexStream == nullptr &&
                    context.preparedIndexMagic == 0 && context.preparedIndexVersion == 0 &&
                    context.preparedEntryCount != nullptr &&
                    *context.preparedEntryCount == 0 &&
                    context.preparedEntryTable == nullptr && context.macroTable == nullptr &&
                    context.macroCount == 0 && context.varTable == nullptr &&
                    context.varCount == 0 && context.ptrArrayHead == nullptr &&
                    context.ptrArrayCount == 0;

    context.Destructor();
    return ok ? 0 : 1;
}

extern "C" int zinterp_scroll_always_callbacks_smoke()
{
    const int oldFrameTick = g_zVideo_FrameTick;
    g_zVideo_FrameTick = 77;

    zModel_InstancePartial instanceA = {};
    zModel_InstancePartial instanceB = {};
    instanceA.scrollingTextureFrameTick = 1;
    instanceB.scrollingTextureFrameTick = 2;

    zClass_NodePartial nodeA = {};
    zClass_NodePartial nodeB = {};
    nodeA.userDataOrDiRef = (unsigned int)&instanceA;
    nodeB.userDataOrDiRef = (unsigned int)&instanceB;

    if (zInterp_Context::DefaultDispatchHook(nullptr) != 0) {
        g_zVideo_FrameTick = oldFrameTick;
        return 1;
    }

    if (zInterp_Object3D::DefaultRenderAction(&nodeA) != 0 ||
        instanceA.scrollingTextureFrameTick != 77) {
        g_zVideo_FrameTick = oldFrameTick;
        return 2;
    }

    zInterp_LinkNode sentinel = {};
    zInterp_LinkNode linkA = {};
    zInterp_LinkNode linkB = {};
    sentinel.next = &linkA;
    sentinel.prev = &linkB;
    linkA.next = &linkB;
    linkA.prev = &sentinel;
    linkA.payload = &nodeA;
    linkB.next = &sentinel;
    linkB.prev = &linkA;
    linkB.payload = &nodeB;

    zInterp_Context context = {};
    context.scrollAlwaysListHead = &sentinel;

    zClass_NodePartial wrapper = {};
    wrapper.callbackContext = (zClass_NodePartial *)&context;

    zInterp_Object3D::ScrollAlwaysTickAction(nullptr);
    zInterp_Object3D::ScrollAlwaysTickAction(&wrapper);

    const bool ok = instanceA.scrollingTextureFrameTick == 77 &&
                    instanceB.scrollingTextureFrameTick == 77;
    g_zVideo_FrameTick = oldFrameTick;
    return ok ? 0 : 3;
}

extern "C" int zinterp_register_scroll_always_node_smoke()
{
    zInterp_LinkNode sentinel = {};
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;

    zInterp_Context context = {};
    context.scrollAlwaysListHead = &sentinel;

    zClass_NodePartial missingDi = {};
    if (context.RegisterScrollAlwaysNode(nullptr, 1.0f, 2, 0) != 0 ||
        context.RegisterScrollAlwaysNode(&missingDi, 1.0f, 2, 0) != 0) {
        return 1;
    }

    zDiPartial directDi = {};
    zClass_NodePartial directNode = {};
    directNode.userDataOrDiRef = (unsigned int)&directDi;
    if (context.RegisterScrollAlwaysNode(&directNode, 2.5f, 7, 0) != 1) {
        return 2;
    }
    if ((directDi.flags & 0x20) == 0 || directDi.textureWorldPerMeter != 2.5f ||
        directDi.textureWorldAxis != 7 ||
        directNode.actionCallback != (void *)(&zInterp_Object3D::DefaultRenderAction) ||
        context.scrollAlwaysListCount != 0) {
        return 3;
    }

    zClass_NodePartial driver = {};
    context.scrollAlwaysDriverNode = &driver;

    zDiPartial queuedDi = {};
    zClass_NodePartial queuedNode = {};
    queuedNode.userDataOrDiRef = (unsigned int)&queuedDi;
    if (context.RegisterScrollAlwaysNode(&queuedNode, 4.0f, 1, 1) != 1) {
        return 4;
    }

    const bool listOk = context.scrollAlwaysListCount == 1 &&
                        sentinel.next == sentinel.prev && sentinel.next != &sentinel &&
                        sentinel.next->prev == &sentinel &&
                        sentinel.next->next == &sentinel &&
                        sentinel.next->payload == &queuedNode;
    if (sentinel.next != &sentinel) {
        delete sentinel.next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    context.scrollAlwaysListCount = 0;

    return listOk && (queuedDi.flags & 0x20) != 0 &&
                   queuedDi.textureWorldPerMeter == 4.0f && queuedDi.textureWorldAxis == 1
               ? 0
               : 5;
}

extern "C" int zinterp_context_eval_condition_expr_smoke()
{
    char aName[] = "A";
    char trueValue[] = "TRUE";
    char bName[] = "B";
    char falseValue[] = "FALSE";
    char cName[] = "C";
    char dName[] = "D";
    zInterp_MacroEntry entries[4] = {
        {aName, trueValue},
        {bName, falseValue},
        {cName, trueValue},
        {dName, falseValue},
    };

    char ifToken[] = "if";
    char orToken[] = "||";
    char andToken[] = "&&";
    char badOpToken[] = "??";

    zInterp_Context single = {};
    single.macroTable = entries;
    single.macroCount = 4;
    single.tokenCount = 2;
    single.tokenList[0] = ifToken;
    single.tokenList[1] = aName;
    const bool singleOk = single.EvalConditionExpr() == 1;

    zInterp_Context leftToRight = {};
    leftToRight.macroTable = entries;
    leftToRight.macroCount = 4;
    leftToRight.tokenCount = 6;
    leftToRight.tokenList[0] = ifToken;
    leftToRight.tokenList[1] = aName;
    leftToRight.tokenList[2] = orToken;
    leftToRight.tokenList[3] = bName;
    leftToRight.tokenList[4] = andToken;
    leftToRight.tokenList[5] = dName;
    const bool leftToRightOk = leftToRight.EvalConditionExpr() == 0;

    zInterp_Context malformed = {};
    malformed.macroTable = entries;
    malformed.macroCount = 4;
    malformed.tokenCount = 4;
    malformed.tokenList[0] = ifToken;
    malformed.tokenList[1] = bName;
    malformed.tokenList[2] = badOpToken;
    malformed.tokenList[3] = cName;
    const bool malformedOk = malformed.EvalConditionExpr() == 0;

    zInterp_Context empty = {};
    empty.tokenCount = 1;
    const bool emptyOk = empty.EvalConditionExpr() == 0;

    return singleOk && leftToRightOk && malformedOk && emptyOk ? 0 : 1;
}

extern "C" int zinterp_context_expand_macro_refs_smoke()
{
    char playerName[] = "PLAYER";
    char playerValue[] = "player1";
    char modeName[] = "MODE";
    char modeValue[] = "hover";
    zInterp_MacroEntry entries[2] = {
        {playerName, playerValue},
        {modeName, modeValue},
    };
    zInterp_Context context = {};
    context.macroTable = entries;
    context.macroCount = 2;

    char noMacro[] = "plain text";
    char *const noMacroResult = context.ExpandMacroRefs(noMacro);
    const bool noMacroOk = noMacroResult == noMacro && std::strcmp(noMacro, "plain text") == 0;

    char missingClose[] = "prefix %PLAYER";
    char *const missingCloseResult = context.ExpandMacroRefs(missingClose);
    const bool missingCloseOk = missingCloseResult == missingClose;

    char expanded[] = "load %PLAYER% as %MODE%";
    char *const expandedResult = context.ExpandMacroRefs(expanded);
    const bool expandedOk =
        expandedResult != expanded && std::strcmp(expandedResult, "load player1 as hover") == 0;

    char stripped[] = "pre%UNKNOWN%mid%PLAYER%post";
    char *const strippedResult = context.ExpandMacroRefs(stripped);
    const bool strippedOk = strippedResult != stripped &&
                            std::strcmp(strippedResult, "premidplayer1post") == 0;

    char trailingUnmatched[] = "%PLAYER% tail %unfinished";
    char *const trailingResult = context.ExpandMacroRefs(trailingUnmatched);
    const bool trailingOk =
        trailingResult != trailingUnmatched &&
        std::strcmp(trailingResult, "player1 tail %unfinished") == 0;

    return noMacroOk && missingCloseOk && expandedOk && strippedOk && trailingOk ? 0 : 1;
}

extern "C" int zinterp_context_next_token_smoke()
{
    char macroName[] = "PLAYER";
    char macroValue[] = "pilot";
    zInterp_MacroEntry entry = {macroName, macroValue};
    char command[] = "cmd";
    char expanded[] = "%PLAYER%";

    zInterp_Context context = {};
    context.macroTable = &entry;
    context.macroCount = 1;
    context.tokenCount = 3;
    context.tokenReadIndex = 1;
    context.tokenList[1] = command;
    context.tokenList[2] = expanded;

    char *const first = context.NextToken();
    const bool firstOk = first == command && context.tokenReadIndex == 2;

    char *const second = context.NextToken();
    const bool secondOk = second != expanded && std::strcmp(second, "pilot") == 0 &&
                          context.tokenReadIndex == 3;

    char *const pastEnd = context.NextToken();
    const bool pastEndOk = pastEnd == nullptr && context.tokenReadIndex == 4;

    zInterp_Context nullSlot = {};
    nullSlot.tokenCount = 1;
    nullSlot.tokenReadIndex = 0;
    nullSlot.tokenList[0] = nullptr;
    char *const nullToken = nullSlot.NextToken();
    const bool nullSlotOk = nullToken == nullptr && nullSlot.tokenReadIndex == 1;

    return firstOk && secondOk && pastEndOk && nullSlotOk ? 0 : 1;
}

extern "C" int zinterp_context_parse_scalar_tokens_smoke()
{
    char intText[] = "-42";
    char floatText[] = "12.5";
    char onText[] = "ON";
    char trueText[] = "true";
    char falseText[] = "off";

    zInterp_Context context = {};
    context.tokenCount = 5;
    context.tokenReadIndex = 0;
    context.tokenList[0] = intText;
    context.tokenList[1] = floatText;
    context.tokenList[2] = onText;
    context.tokenList[3] = trueText;
    context.tokenList[4] = falseText;

    const bool intOk = context.ParseIntToken() == -42 && context.tokenReadIndex == 1;
    const float parsedFloat = context.ParseFloatToken();
    const bool floatOk = parsedFloat > 12.49f && parsedFloat < 12.51f &&
                         context.tokenReadIndex == 2;
    const bool onOk = context.ParseBoolToken() == 1 && context.tokenReadIndex == 3;
    const bool trueOk = context.ParseBoolToken() == 1 && context.tokenReadIndex == 4;
    const bool falseOk = context.ParseBoolToken() == 0 && context.tokenReadIndex == 5;

    const bool emptyOk = context.ParseIntToken() == 0 &&
                         context.ParseFloatToken() == 0.0f &&
                         context.ParseBoolToken() == 0 && context.tokenReadIndex == 8;

    return intOk && floatOk && onOk && trueOk && falseOk && emptyOk ? 0 : 1;
}

extern "C" int zinterp_context_var_entry_helpers_smoke()
{
    char healthName[] = "health";
    char speedName[] = "speed";
    char labelName[] = "label";
    int health = 77;
    float speed = 3.25f;
    char label[] = "alpha";
    zInterp_VarEntry entries[3] = {
        {healthName, 0, {&health}},
        {speedName, 1, {nullptr}},
        {labelName, 2, {nullptr}},
    };
    entries[1].valuePtr.floatPtr = &speed;
    entries[2].valuePtr.charPtr = label;

    zInterp_Context context = {};
    context.varTable = entries;
    context.varCount = 3;

    const bool findOk = context.FindVarEntry("speed") == &entries[1] &&
                        context.FindVarEntry("missing") == nullptr;

    zInterp_Context empty = {};
    const bool emptyOk = empty.FindVarEntry("health") == nullptr;

    context.logFn = TestLogFn;
    g_logCallCount = 0;
    context.DumpVarEntry(nullptr);
    const bool nullDumpOk = g_logCallCount == 0;

    context.DumpVarEntry(&entries[0]);
    const bool intDumpOk = g_logCallCount == 1 &&
                           std::strcmp(g_logFmt, "%s = (int) %d") == 0 &&
                           g_logNameArg == healthName && g_logSecondInt == 77;

    context.DumpVarEntry(&entries[1]);
    const bool floatDumpOk = g_logCallCount == 2 &&
                             std::strcmp(g_logFmt, "%s = (float) %f") == 0 &&
                             g_logNameArg == speedName && g_logSecondDouble > 3.24 &&
                             g_logSecondDouble < 3.26;

    context.DumpVarEntry(&entries[2]);
    const bool stringDumpOk = g_logCallCount == 3 &&
                              std::strcmp(g_logFmt, "%s = (char*)\"%s\"") == 0 &&
                              g_logNameArg == labelName && g_logSecondInt == 'a';

    return findOk && emptyOk && nullDumpOk && intDumpOk && floatDumpOk && stringDumpOk ? 0 : 1;
}

extern "C" int zinterp_context_command_helpers_smoke()
{
    char command[] = "texture";
    zInterp_Context context = {};
    context.tokenCount = 1;
    context.tokenList[0] = command;

    const bool currentOk = context.GetCurrentCommand() == command;
    const bool equalsOk = context.CommandEquals("texture") == 1 &&
                          context.CommandEquals("tex") == 0 &&
                          context.CommandEquals("texture2") == 0;
    const bool prefixOk = context.CommandEqualsPrefix("tex", 3) == 1 &&
                          context.CommandEqualsPrefix("texture", 7) == 1 &&
                          context.CommandEqualsPrefix("textured", 8) == 0;

    zInterp_Context empty = {};
    const bool emptyCurrentOk = empty.GetCurrentCommand() == nullptr;

    return currentOk && equalsOk && prefixOk && emptyCurrentOk ? 0 : 1;
}

extern "C" int zinterp_context_validate_args_and_node_type_smoke()
{
    char command[] = "object3d";
    zInterp_Context context = {};
    context.logFn = TestLogFn;
    context.tokenCount = 2;
    context.tokenList[0] = command;

    zClass_NodePartial node = {};
    node.classId = 5;

    g_logCallCount = 0;
    const bool successOk = context.ValidateArgsAndNodeType(1, 5, &node) == 1 &&
                           g_logCallCount == 0 && context.lineHadError == 0;

    context.lineHadError = 0;
    g_logCallCount = 0;
    g_logFmt = nullptr;
    g_logNameArg = nullptr;
    const bool nullNodeOk =
        context.ValidateArgsAndNodeType(1, 5, nullptr) == 0 && context.lineHadError == 1 &&
        g_logCallCount == 1 && std::strcmp(g_logFmt,
                                           "Interp: keyword [%s] has NULL node to work with") == 0 &&
        g_logNameArg == command;

    context.lineHadError = 0;
    g_logCallCount = 0;
    g_logFmt = nullptr;
    node.classId = 3;
    const bool classMismatchOk =
        context.ValidateArgsAndNodeType(1, 5, &node) == 0 && context.lineHadError == 1 &&
        g_logCallCount == 1 &&
        std::strcmp(g_logFmt,
                    "Interp: keyword [%s] has node [%s] of class=%d, expected=%d") == 0 &&
        g_logNameArg == command && g_logThirdInt == 3 && g_logFourthInt == 5;

    context.lineHadError = 0;
    g_logCallCount = 0;
    g_logFmt = nullptr;
    node.classId = 5;
    const bool argCountOk =
        context.ValidateArgsAndNodeType(2, 5, &node) == 0 && context.lineHadError == 1 &&
        g_logCallCount == 1 &&
        std::strcmp(g_logFmt,
                    "Interp: keyword [%s] requires (%d) args, found (%d) args") == 0 &&
        g_logNameArg == command && g_logSecondInt == 2 && g_logThirdInt == 1;

    return successOk && nullNodeOk && classMismatchOk && argCountOk ? 0 : 1;
}

extern "C" int zinterp_context_tokenize_line_smoke()
{
    zInterp_Context context = {};
    const int result = context.TokenizeLine("  command, arg1\targ2\ntrailing # comment");
    const bool ok = result == 1 && context.tokenReadIndex == 1 && context.tokenCount == 4 &&
                    context.tempAlloc != nullptr &&
                    std::strcmp(context.tokenList[0], "command") == 0 &&
                    std::strcmp(context.tokenList[1], "arg1") == 0 &&
                    std::strcmp(context.tokenList[2], "arg2") == 0 &&
                    std::strcmp(context.tokenList[3], "trailing ") == 0;

    std::free(context.tempAlloc);
    return ok ? 0 : 1;
}

extern "C" int zinterp_context_tokenize_comment_and_prepared_smoke()
{
    zInterp_Context commentContext = {};
    const int commentResult = commentContext.TokenizeLine("cmd arg # ignored");
    const bool commentOk =
        commentResult == 1 && commentContext.tokenCount == 2 &&
        std::strcmp(commentContext.tokenList[0], "cmd") == 0 &&
        std::strcmp(commentContext.tokenList[1], "arg") == 0;

    zInterp_Context preparedContext = {};
    preparedContext.hasPreparedInput = 1;
    preparedContext.tokenCount = 7;
    preparedContext.tokenReadIndex = 3;
    const int preparedResult = preparedContext.TokenizeLine("should not parse");
    const bool preparedOk = preparedResult == 1 && preparedContext.tokenCount == 7 &&
                            preparedContext.tokenReadIndex == 3 &&
                            preparedContext.tempAlloc == nullptr;

    std::free(commentContext.tempAlloc);
    return commentOk && preparedOk ? 0 : 2;
}

extern "C" int zinterp_context_echo_tokens_smoke()
{
    zInterp_Context context = {};
    char command[] = "echo";
    char argument[] = "value";
    context.tokenCount = 2;
    context.tokenList[0] = command;
    context.tokenList[1] = argument;

    FILE *const capture = std::tmpfile();
    if (capture == nullptr) {
        return 1;
    }

    std::fflush(stdout);
    const int savedStdout = _dup(_fileno(stdout));
    if (savedStdout < 0) {
        std::fclose(capture);
        return 2;
    }

    if (_dup2(_fileno(capture), _fileno(stdout)) != 0) {
        _close(savedStdout);
        std::fclose(capture);
        return 3;
    }

    const int result = context.EchoTokens();
    std::fflush(stdout);

    char output[32] = {};
    std::fseek(capture, 0, SEEK_SET);
    const size_t bytesRead = std::fread(output, 1, sizeof(output) - 1, capture);

    _dup2(savedStdout, _fileno(stdout));
    _close(savedStdout);
    std::fclose(capture);

    return result == 1 && bytesRead == 12 && std::strcmp(output, "echo value \n") == 0 ? 0 : 4;
}

extern "C" int zinterp_context_push_file_frame_smoke()
{
    zInterp_Context context = {};
    unsigned char fileSentinelA = 0;
    unsigned char fileSentinelB = 0;
    FILE *const fileA = reinterpret_cast<FILE *>(&fileSentinelA);
    FILE *const fileB = reinterpret_cast<FILE *>(&fileSentinelB);

    if (context.PushFileFrame(fileA, 11, 0) != 1 || context.fileFrameCount != 1 ||
        context.fileFrameStack == nullptr) {
        std::free(context.fileFrameStack);
        return 1;
    }

    if (context.fileFrameStack[0].file != fileA || context.fileFrameStack[0].filePos != 11 ||
        context.fileFrameStack[0].hasPreparedInput != 0) {
        std::free(context.fileFrameStack);
        return 2;
    }

    if (context.PushFileFrame(fileB, 22, 1) != 1 || context.fileFrameCount != 2 ||
        context.fileFrameStack == nullptr) {
        std::free(context.fileFrameStack);
        return 3;
    }

    const bool ok = context.fileFrameStack[0].file == fileA &&
                    context.fileFrameStack[0].filePos == 11 &&
                    context.fileFrameStack[0].hasPreparedInput == 0 &&
                    context.fileFrameStack[1].file == fileB &&
                    context.fileFrameStack[1].filePos == 22 &&
                    context.fileFrameStack[1].hasPreparedInput == 1;

    std::free(context.fileFrameStack);
    return ok ? 0 : 4;
}

extern "C" int zinterp_context_pop_file_frame_smoke()
{
    zInterp_FileFrame frames[2] = {};
    frames[0].filePos = 11;
    frames[1].filePos = 22;

    zInterp_Context context = {};
    context.fileFrameStack = frames;
    context.fileFrameCount = 2;

    zInterp_FileFrame *const top = context.PopFileFrame();
    const bool firstPopOk = top == &frames[1] && context.fileFrameCount == 1 && top->filePos == 22;

    zInterp_FileFrame *const next = context.PopFileFrame();
    const bool secondPopOk =
        next == &frames[0] && context.fileFrameCount == 0 && next->filePos == 11;

    zInterp_FileFrame *const empty = context.PopFileFrame();
    return firstPopOk && secondPopOk && empty == nullptr && context.fileFrameCount == 0 ? 0 : 1;
}

extern "C" int zinterp_context_print_node_tree_smoke()
{
    zClass_NodePartial root = {};
    zClass_NodePartial childA = {};
    zClass_NodePartial childB = {};
    zClass_NodePartial grandchild = {};
    zClass_NodePartial *rootChildren[] = {&childA, &childB};
    zClass_NodePartial *childBChildren[] = {&grandchild};

    std::strcpy(root.name, "root");
    std::strcpy(childA.name, "left");
    std::strcpy(childB.name, "right");
    std::strcpy(grandchild.name, "leaf");
    root.listCountB = 2;
    root.listB = rootChildren;
    childB.listCountB = 1;
    childB.listB = childBChildren;

    zInterp_Context context = {};
    context.logFn = TestLogFn;
    g_logCallCount = 0;
    context.PrintNodeTree(nullptr, 4);
    if (g_logCallCount != 0) {
        return 1;
    }

    context.PrintNodeTree(&root, 2);
    return g_logCallCount == 4 && g_logFmt != nullptr &&
                   std::strcmp(g_logFmt, "%*s%s") == 0 && g_logValue == 6 &&
                   g_logNameArg != nullptr && g_logThirdNameArg == grandchild.name
               ? 0
               : 2;
}

extern "C" int zinterp_context_read_text_line_smoke()
{
    char tempDir[MAX_PATH] = {};
    char scriptPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zln", 0, scriptPath) == 0) {
        return 1;
    }

    FILE *const writeFile = std::fopen(scriptPath, "wb");
    if (writeFile == nullptr) {
        DeleteFileA(scriptPath);
        return 2;
    }
    std::fputs("alpha\nbeta", writeFile);
    std::fclose(writeFile);

    FILE *const scriptFile = std::fopen(scriptPath, "rb");
    if (scriptFile == nullptr) {
        DeleteFileA(scriptPath);
        return 3;
    }

    zInterp_Context context = {};
    char lineBuffer[32] = {};
    const int read = context.ReadLineOrPreparedTokens(scriptFile, lineBuffer);
    const bool ok = read == 1 && std::memcmp(lineBuffer, "alpha\n", 6) == 0;

    std::fclose(scriptFile);
    DeleteFileA(scriptPath);
    return ok ? 0 : 4;
}

extern "C" int zinterp_context_read_prepared_tokens_smoke()
{
    char tempDir[MAX_PATH] = {};
    char packetPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "ztk", 0, packetPath) == 0) {
        return 1;
    }

    FILE *const writeFile = std::fopen(packetPath, "wb");
    if (writeFile == nullptr) {
        DeleteFileA(packetPath);
        return 2;
    }
    const char tokenBlob[] = {'c', 'm', 'd', '\0', 'a', 'r', 'g', '\0'};
    WriteU32(writeFile, sizeof(tokenBlob));
    WriteU32(writeFile, 2);
    std::fwrite(tokenBlob, sizeof(tokenBlob), 1, writeFile);
    std::fclose(writeFile);

    FILE *const packetFile = std::fopen(packetPath, "rb");
    if (packetFile == nullptr) {
        DeleteFileA(packetPath);
        return 3;
    }

    zInterp_Context context = {};
    context.hasPreparedInput = 1;
    char lineBuffer[4] = {};
    const int read = context.ReadLineOrPreparedTokens(packetFile, lineBuffer);
    const bool ok = read == 1 && context.tokenReadIndex == 1 && context.tokenCount == 2 &&
                    context.tempAlloc != nullptr && context.tokenList[0] == context.tempAlloc &&
                    std::strcmp(context.tokenList[0], "cmd") == 0 &&
                    std::strcmp(context.tokenList[1], "arg") == 0;

    std::free(context.tempAlloc);
    std::fclose(packetFile);
    DeleteFileA(packetPath);
    return ok ? 0 : 4;
}

extern "C" int zinterp_context_read_prepared_empty_packet_smoke()
{
    char tempDir[MAX_PATH] = {};
    char packetPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zte", 0, packetPath) == 0) {
        return 1;
    }

    FILE *const writeFile = std::fopen(packetPath, "wb");
    if (writeFile == nullptr) {
        DeleteFileA(packetPath);
        return 2;
    }
    WriteU32(writeFile, 0);
    std::fclose(writeFile);

    FILE *const packetFile = std::fopen(packetPath, "rb");
    if (packetFile == nullptr) {
        DeleteFileA(packetPath);
        return 3;
    }

    zInterp_Context context = {};
    context.hasPreparedInput = 1;
    context.tokenCount = 99;
    context.tempAlloc = reinterpret_cast<char *>(1);
    char lineBuffer[4] = {};
    const int read = context.ReadLineOrPreparedTokens(packetFile, lineBuffer);
    const bool ok =
        read == 0 && context.tokenReadIndex == 1 && context.tokenCount == 0 &&
        context.tempAlloc == nullptr;

    std::fclose(packetFile);
    DeleteFileA(packetPath);
    return ok ? 0 : 4;
}

extern "C" int zinterp_context_load_prepared_script_index_smoke()
{
    TestZrdrFreePoolScope freePoolScope;
    char tempDir[MAX_PATH] = {};
    char indexPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zip", 0, indexPath) == 0) {
        return 1;
    }

    zInterp_PreparedScriptEntry entry = {};
    std::strcpy(entry.path, "missing_source_for_prepared_index.gw");
    entry.fileTime = 123;
    entry.fileOffset = 456;
    if (!WritePreparedIndex(indexPath, &entry, 1)) {
        DeleteFileA(indexPath);
        return 2;
    }

    int entryCount = 0;
    zInterp_Context context = {};
    context.searchPathSpec = tempDir;
    context.preparedEntryCount = &entryCount;

    const int loaded = context.LoadPreparedScriptIndex(BaseName(indexPath));
    const bool loadedOk =
        loaded == 1 && context.preparedIndexStream != nullptr &&
        context.preparedIndexMagic == 0x08971119 && context.preparedIndexVersion == 7 &&
        entryCount == 1 && context.preparedEntryTable != nullptr &&
        std::strcmp(context.preparedEntryTable[0].path, entry.path) == 0 &&
        context.preparedEntryTable[0].fileTime == 123 &&
        context.preparedEntryTable[0].fileOffset == 456;

    const int earlyLoaded = context.LoadPreparedScriptIndex("not_used.zbd");
    const bool earlyOk = earlyLoaded == 1 && entryCount == 1 && context.preparedIndexStream != nullptr;

    if (context.preparedIndexStream != nullptr) {
        std::fclose(context.preparedIndexStream);
    }
    std::free(context.preparedEntryTable);
    if (context.archiveSearchList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(context.archiveSearchList);
    }
    DeleteFileA(indexPath);
    return loadedOk && earlyOk ? 0 : 3;
}

extern "C" int zinterp_context_load_prepared_script_index_stale_smoke()
{
    TestZrdrFreePoolScope freePoolScope;
    char tempDir[MAX_PATH] = {};
    char indexPath[MAX_PATH] = {};
    char sourcePath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zip", 0, indexPath) == 0 ||
        GetTempFileNameA(tempDir, "zsp", 0, sourcePath) == 0) {
        return 1;
    }

    FILE *const sourceFile = std::fopen(sourcePath, "wb");
    if (sourceFile == nullptr) {
        DeleteFileA(indexPath);
        DeleteFileA(sourcePath);
        return 2;
    }
    std::fputc('x', sourceFile);
    std::fclose(sourceFile);

    zInterp_PreparedScriptEntry entry = {};
    std::strncpy(entry.path, sourcePath, sizeof(entry.path) - 1);
    entry.fileTime = 0;
    entry.fileOffset = 64;
    if (!WritePreparedIndex(indexPath, &entry, 1)) {
        DeleteFileA(indexPath);
        DeleteFileA(sourcePath);
        return 3;
    }

    int entryCount = 77;
    zInterp_Context context = {};
    context.searchPathSpec = tempDir;
    context.preparedEntryCount = &entryCount;

    const int loaded = context.LoadPreparedScriptIndex(BaseName(indexPath));
    const bool staleOk = loaded == 0 && context.preparedIndexStream == nullptr &&
                         context.preparedEntryTable == nullptr && entryCount == 77;

    if (context.archiveSearchList != nullptr) {
        zUtil_ZRDR_FreeSearchPathList(context.archiveSearchList);
    }
    DeleteFileA(indexPath);
    DeleteFileA(sourcePath);
    return staleOk ? 0 : 4;
}

extern "C" int zinterp_context_open_prepared_script_stream_smoke()
{
    char tempDir[MAX_PATH] = {};
    char preparedPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zps", 0, preparedPath) == 0) {
        return 1;
    }

    FILE *const writeFile = std::fopen(preparedPath, "wb");
    if (writeFile == nullptr) {
        DeleteFileA(preparedPath);
        return 2;
    }
    for (int i = 0; i < 32; ++i) {
        std::fputc('A' + i, writeFile);
    }
    std::fclose(writeFile);

    FILE *const preparedStream = std::fopen(preparedPath, "rb");
    if (preparedStream == nullptr) {
        DeleteFileA(preparedPath);
        return 3;
    }

    int entryCount = 2;
    zInterp_PreparedScriptEntry entries[2] = {};
    std::strcpy(entries[0].path, "support\\common.gw");
    entries[0].fileTime = 10;
    entries[0].fileOffset = 7;
    std::strcpy(entries[1].path, "support\\other.gw");
    entries[1].fileTime = 20;
    entries[1].fileOffset = 13;

    zInterp_Context context = {};
    context.preparedIndexStream = preparedStream;
    context.preparedEntryCount = &entryCount;
    context.preparedEntryTable = entries;

    FILE *const opened = context.OpenPreparedScriptStream("SUPPORT\\COMMON.GW");
    const bool openOk = opened == preparedStream && std::ftell(preparedStream) == 7;
    const bool missingOk = context.OpenPreparedScriptStream("missing.gw") == nullptr;

    std::fclose(preparedStream);
    DeleteFileA(preparedPath);
    return openOk && missingOk ? 0 : 4;
}

extern "C" int zinterp_context_open_prepared_script_stream_newer_source_smoke()
{
    char tempDir[MAX_PATH] = {};
    char preparedPath[MAX_PATH] = {};
    char sourcePath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zps", 0, preparedPath) == 0 ||
        GetTempFileNameA(tempDir, "zsc", 0, sourcePath) == 0) {
        return 1;
    }

    FILE *const writeFile = std::fopen(preparedPath, "wb");
    if (writeFile == nullptr) {
        DeleteFileA(preparedPath);
        DeleteFileA(sourcePath);
        return 2;
    }
    std::fputs("prepared", writeFile);
    std::fclose(writeFile);

    FILE *const sourceFile = std::fopen(sourcePath, "wb");
    if (sourceFile == nullptr) {
        DeleteFileA(preparedPath);
        DeleteFileA(sourcePath);
        return 3;
    }
    std::fputs("source", sourceFile);
    std::fclose(sourceFile);

    FILE *const preparedStream = std::fopen(preparedPath, "rb");
    if (preparedStream == nullptr) {
        DeleteFileA(preparedPath);
        DeleteFileA(sourcePath);
        return 4;
    }

    int entryCount = 1;
    zInterp_PreparedScriptEntry entry = {};
    std::strncpy(entry.path, sourcePath, sizeof(entry.path) - 1);
    entry.fileTime = 0;
    entry.fileOffset = 1;

    zInterp_Context context = {};
    context.preparedIndexStream = preparedStream;
    context.preparedEntryCount = &entryCount;
    context.preparedEntryTable = &entry;

    const bool newerSourceOk = context.OpenPreparedScriptStream(sourcePath) == nullptr &&
                               std::ftell(preparedStream) == 0;

    std::fclose(preparedStream);
    DeleteFileA(preparedPath);
    DeleteFileA(sourcePath);
    return newerSourceOk ? 0 : 5;
}

extern "C" int zinterp_context_handle_builtin_command_smoke()
{
    zInterp_Context context = {};

    char setCommand[] = "set";
    char macroName[] = "ENABLED";
    char macroValue[] = "TRUE";
    context.tokenCount = 3;
    context.tokenReadIndex = 1;
    context.tokenList[0] = setCommand;
    context.tokenList[1] = macroName;
    context.tokenList[2] = macroValue;
    const bool setOk = context.HandleBuiltinCommand(setCommand) == 0 &&
                       context.macroCount == 1 &&
                       std::strcmp(context.macroTable[0].name, "ENABLED") == 0 &&
                       std::strcmp(context.macroTable[0].value, "TRUE") == 0;

    char ifndefCommand[] = "ifndef";
    context.tokenCount = 2;
    context.tokenReadIndex = 1;
    context.tokenList[0] = ifndefCommand;
    context.tokenList[1] = macroName;
    const bool ifndefOk = context.HandleBuiltinCommand(ifndefCommand) == 0 &&
                          context.conditionalDepth == 1;

    char skippedCommand[] = "set";
    char skippedName[] = "SKIPPED";
    char skippedValue[] = "TRUE";
    context.tokenCount = 3;
    context.tokenReadIndex = 1;
    context.tokenList[0] = skippedCommand;
    context.tokenList[1] = skippedName;
    context.tokenList[2] = skippedValue;
    const bool skipOk = context.HandleBuiltinCommand(skippedCommand) == 0 &&
                        context.macroCount == 1;

    char endifCommand[] = "endif";
    context.tokenCount = 1;
    context.tokenReadIndex = 1;
    context.tokenList[0] = endifCommand;
    const bool endifOk = context.HandleBuiltinCommand(endifCommand) == 0 &&
                         context.conditionalDepth == 0;

    int intValue = 3;
    float floatValue = 2.0f;
    zInterp_VarEntry vars[2] = {
        {_strdup("COUNT"), 0, {&intValue}},
        {_strdup("SCALE"), 1, {nullptr}},
    };
    vars[1].valuePtr.floatPtr = &floatValue;
    context.varTable = vars;
    context.varCount = 2;

    char varCommand[] = "var";
    char countName[] = "COUNT";
    char setToken[] = "set";
    char intText[] = "17";
    context.tokenCount = 4;
    context.tokenReadIndex = 1;
    context.tokenList[0] = varCommand;
    context.tokenList[1] = countName;
    context.tokenList[2] = setToken;
    context.tokenList[3] = intText;
    const bool intVarOk = context.HandleBuiltinCommand(varCommand) == 0 && intValue == 17;

    char scaleName[] = "SCALE";
    char equalToken[] = "=";
    char floatText[] = "4.5";
    context.tokenCount = 4;
    context.tokenReadIndex = 1;
    context.tokenList[0] = varCommand;
    context.tokenList[1] = scaleName;
    context.tokenList[2] = equalToken;
    context.tokenList[3] = floatText;
    const bool floatVarOk = context.HandleBuiltinCommand(varCommand) == 0 &&
                            floatValue > 4.49f && floatValue < 4.51f;

    char quitCommand[] = "Quit";
    context.tokenCount = 1;
    context.tokenReadIndex = 1;
    context.tokenList[0] = quitCommand;
    const bool quitOk = context.HandleBuiltinCommand(quitCommand) == 0 &&
                        context.parseResult == 1;

    char unknownCommand[] = "NotBuiltin";
    context.parseResult = 0;
    const bool unknownOk = context.HandleBuiltinCommand(unknownCommand) == 1;

    context.ClearMacroTable();
    std::free(vars[0].name);
    std::free(vars[1].name);

    if (!setOk) {
        return 1;
    }
    if (!ifndefOk) {
        return 2;
    }
    if (!skipOk) {
        return 3;
    }
    if (!endifOk) {
        return 4;
    }
    if (!intVarOk) {
        return 5;
    }
    if (!floatVarOk) {
        return 6;
    }
    if (!quitOk) {
        return 7;
    }
    if (!unknownOk) {
        return 8;
    }
    return 0;
}

extern "C" int zinterp_context_run_stream_builtin_smoke()
{
    zInterp_Context context = {};
    char setLine[] = "set STREAM_OK TRUE";
    const int setResult = context.RunStream(setLine);
    const bool setOk = setResult == 1 && context.macroCount == 1 &&
                       std::strcmp(context.macroTable[0].name, "STREAM_OK") == 0 &&
                       std::strcmp(context.macroTable[0].value, "TRUE") == 0 &&
                       context.tempAlloc == nullptr && context.parseResult == 0;

    char quitLine[] = "Quit";
    const int quitResult = context.RunStream(quitLine);
    const bool quitOk = quitResult == 0 && context.parseResult == 1 &&
                        context.tempAlloc == nullptr;

    context.ClearMacroTable();
    if (!setOk) {
        return 1;
    }
    if (!quitOk) {
        return 2;
    }
    return 0;
}

extern "C" int zinterp_context_run_script_file_nested_source_smoke()
{
    char tempDir[MAX_PATH];
    char mainPath[MAX_PATH];
    char includePath[MAX_PATH];
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "rci", 0, includePath) == 0 ||
        GetTempFileNameA(tempDir, "rcm", 0, mainPath) == 0) {
        return 1;
    }

    FILE *const includeFile = std::fopen(includePath, "wb");
    if (includeFile == nullptr) {
        DeleteFileA(includePath);
        DeleteFileA(mainPath);
        return 2;
    }
    std::fputs("set INCLUDED TRUE\n", includeFile);
    std::fclose(includeFile);

    FILE *const mainFile = std::fopen(mainPath, "wb");
    if (mainFile == nullptr) {
        DeleteFileA(includePath);
        DeleteFileA(mainPath);
        return 3;
    }
    std::fprintf(mainFile, "set MAIN TRUE\nsource %s\nQuit\n", includePath);
    std::fclose(mainFile);

    zInterp_Context context = {};
    const int runResult = context.RunScriptFile(mainPath);
    const bool resultOk = runResult == 1;
    const bool currentFileOk = context.currentScriptFile == nullptr;
    const bool preparedOk = context.hasPreparedInput == 0;
    const bool frameOk = context.fileFrameCount == 0;
    const bool mainMacroOk = context.FindMacroValue("MAIN", nullptr) != nullptr;
    const bool includedMacroOk = context.FindMacroValue("INCLUDED", nullptr) != nullptr;

    context.ClearMacroTable();
    context.ClearFileFrameStack();
    DeleteFileA(includePath);
    DeleteFileA(mainPath);
    if (!resultOk) {
        return 4;
    }
    if (!currentFileOk) {
        return 5;
    }
    if (!preparedOk) {
        return 6;
    }
    if (!frameOk) {
        return 7;
    }
    if (!mainMacroOk) {
        return 8;
    }
    if (!includedMacroOk) {
        return 9;
    }
    return 0;
}

extern "C" int zinterp_context_dispatch_core_node_flags_smoke()
{
    zInterp_Context context = {};
    zClass_NodePartial node = {};
    node.classId = 6;
    context.currentNode = &node;

    char onText[] = "on";
    char offText[] = "off";

    char altitudeCommand[] = "SetAltitudeSurface";
    char *altitudeTokens[] = {altitudeCommand, onText};
    SetCommandTokens(&context, altitudeTokens, 2);
    const bool altitudeOk = context.DispatchCoreCommand(altitudeCommand) == 1 &&
                            (node.flags & 0x08) != 0;

    char bboxCommand[] = "SetIntersectBBOX";
    char *bboxTokens[] = {bboxCommand, onText};
    SetCommandTokens(&context, bboxTokens, 2);
    const bool bboxOk = context.DispatchCoreCommand(bboxCommand) == 1 &&
                        (node.flags & 0x20) != 0;

    char surfaceCommand[] = "SetIntersectSurface";
    char *surfaceTokens[] = {surfaceCommand, onText};
    SetCommandTokens(&context, surfaceTokens, 2);
    const bool surfaceOk = context.DispatchCoreCommand(surfaceCommand) == 1 &&
                           (node.flags & 0x10) != 0;

    char landmarkCommand[] = "SetLandmark";
    char *landmarkTokens[] = {landmarkCommand, onText};
    SetCommandTokens(&context, landmarkTokens, 2);
    const bool landmarkOk = context.DispatchCoreCommand(landmarkCommand) == 1 &&
                            (node.flags & 0x80) != 0;

    char proximityCommand[] = "SetProximity";
    char *proximityTokens[] = {proximityCommand, onText};
    SetCommandTokens(&context, proximityTokens, 2);
    const bool proximityOk = context.DispatchCoreCommand(proximityCommand) == 1 &&
                             (node.flags & 0x40) != 0;

    char clearSurfaceCommand[] = "SetIntersectSurface";
    char *clearSurfaceTokens[] = {clearSurfaceCommand, offText};
    SetCommandTokens(&context, clearSurfaceTokens, 2);
    const bool clearSurfaceOk = context.DispatchCoreCommand(clearSurfaceCommand) == 1 &&
                                (node.flags & 0x10) == 0;

    char nodeActiveCommand[] = "NodeSetActive";
    char *nodeActiveTokens[] = {nodeActiveCommand, onText};
    SetCommandTokens(&context, nodeActiveTokens, 2);
    const bool nodeActiveOk = context.DispatchCoreCommand(nodeActiveCommand) == 1 &&
                              (node.flags & 0x04) != 0;

    char descriptionCommand[] = "NodeSetDescription";
    char descriptionText[] = "node_desc";
    char *descriptionTokens[] = {descriptionCommand, descriptionText};
    SetCommandTokens(&context, descriptionTokens, 2);
    const bool descriptionOk = context.DispatchCoreCommand(descriptionCommand) == 1 &&
                               std::strcmp(node.name, "node_desc") == 0;

    char modifyCommand[] = "NodeSetCanModify";
    char *modifyTokens[] = {modifyCommand, onText};
    SetCommandTokens(&context, modifyTokens, 2);
    const bool modifyOk = context.DispatchCoreCommand(modifyCommand) == 1 &&
                          (node.flags & 0x00010000) != 0;

    char overwriteCommand[] = "NodeSetOverwrite";
    char *overwriteTokens[] = {overwriteCommand, onText};
    SetCommandTokens(&context, overwriteTokens, 2);
    const bool overwriteOk = context.DispatchCoreCommand(overwriteCommand) == 1 &&
                             (node.flags & 0x00800000) != 0;

    return altitudeOk && bboxOk && surfaceOk && landmarkOk && proximityOk && clearSurfaceOk &&
                   nodeActiveOk && descriptionOk && modifyOk && overwriteOk
               ? 0
               : 1;
}

extern "C" int zinterp_context_dispatch_core_camera_clip_smoke()
{
    zInterp_Context context = {};
    zClass_CameraDataPartial cameraData = {};
    cameraData.nearClip = 1.0f;
    cameraData.farClip = 100.0f;
    cameraData.clipDistance = 50.0f;

    zClass_NodePartial camera = {};
    camera.classId = 1;
    camera.classData = &cameraData;
    context.currentNode = &camera;

    char nearCommand[] = "CameraSetNearClip";
    char nearText[] = "2.5";
    char *nearTokens[] = {nearCommand, nearText};
    SetCommandTokens(&context, nearTokens, 2);
    const bool nearOk = context.DispatchCoreCommand(nearCommand) == 1 &&
                        cameraData.nearClip > 2.49f && cameraData.nearClip < 2.51f &&
                        cameraData.farClip == 100.0f;

    char farCommand[] = "CameraSetFarClip";
    char farText[] = "250";
    char *farTokens[] = {farCommand, farText};
    SetCommandTokens(&context, farTokens, 2);
    const bool farOk = context.DispatchCoreCommand(farCommand) == 1 &&
                       cameraData.nearClip > 2.49f && cameraData.nearClip < 2.51f &&
                       cameraData.farClip > 249.9f && cameraData.farClip < 250.1f;

    char lodCommand[] = "CameraSetLODMultiplier";
    char clipText[] = "80";
    char *lodTokens[] = {lodCommand, clipText};
    SetCommandTokens(&context, lodTokens, 2);
    const bool lodOk = context.DispatchCoreCommand(lodCommand) == 1 &&
                       cameraData.clipDistance > 79.9f &&
                       cameraData.clipDistance < 80.1f &&
                       cameraData.invClipDistanceSq > 0.00015f &&
                       cameraData.invClipDistanceSq < 0.00016f;

    return nearOk && farOk && lodOk ? 0 : 1;
}

extern "C" int zinterp_context_dispatch_core_world_and_globals_smoke()
{
    zInterp_Context context = {};
    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classId = 2;
    std::strcpy(world.name, "world");
    world.classData = &worldData;
    context.currentNode = &world;

    char extentsCommand[] = "WorldExtents";
    char sizeX[] = "400";
    char sizeZ[] = "600";
    char *extentsTokens[] = {extentsCommand, sizeX, sizeZ};
    SetCommandTokens(&context, extentsTokens, 3);
    const bool extentsOk = context.DispatchCoreCommand(extentsCommand) == 1 &&
                           worldData.worldSizeX == 400.0f &&
                           worldData.worldSizeZ == 600.0f;

    char originCommand[] = "WorldOrigin";
    char originX[] = "10";
    char originZ[] = "20";
    char *originTokens[] = {originCommand, originX, originZ};
    SetCommandTokens(&context, originTokens, 3);
    const bool originOk = context.DispatchCoreCommand(originCommand) == 1 &&
                          worldData.originX == 10.0f && worldData.originZ == 20.0f &&
                          worldData.worldMaxX == 410.0f && worldData.worldMaxZ == 620.0f;

    char fogRangeCommand[] = "WorldSetFogRange";
    char fogNear[] = "30";
    char fogFar[] = "300";
    char *fogRangeTokens[] = {fogRangeCommand, fogNear, fogFar};
    SetCommandTokens(&context, fogRangeTokens, 3);
    const bool fogRangeOk = context.DispatchCoreCommand(fogRangeCommand) == 1 &&
                            worldData.fogDistanceStart == 30.0f &&
                            worldData.fogDistanceEnd == 300.0f;

    char fogNearCommand[] = "WorldSetFogRangeNear";
    char fogNear2[] = "40";
    char *fogNearTokens[] = {fogNearCommand, fogNear2};
    SetCommandTokens(&context, fogNearTokens, 2);
    const bool fogNearOk = context.DispatchCoreCommand(fogNearCommand) == 1 &&
                           worldData.fogDistanceStart == 40.0f &&
                           worldData.fogDistanceEnd == 300.0f;

    char fogStateCommand[] = "WorldSetFogState";
    char linearText[] = "linear";
    char *fogStateTokens[] = {fogStateCommand, linearText};
    SetCommandTokens(&context, fogStateTokens, 2);
    const bool fogStateOk = context.DispatchCoreCommand(fogStateCommand) == 1 &&
                            worldData.fogState == 1;

    char rejectCommand[] = "SetSmallPolygonRejectArea";
    char rejectArea[] = "5";
    char *rejectTokens[] = {rejectCommand, rejectArea};
    SetCommandTokens(&context, rejectTokens, 2);
    const bool rejectOk = context.DispatchCoreCommand(rejectCommand) == 1 &&
                          gModel_SmallPolyRejectArea2x == 10.0f &&
                          gModel_SmallPolyRejectArea20x == 100.0f;

    char writeTypeCommand[] = "WriteTextureSetType";
    char slotText[] = "2";
    char *writeTypeTokens[] = {writeTypeCommand, slotText};
    SetCommandTokens(&context, writeTypeTokens, 2);
    const bool writeTypeOk = context.DispatchCoreCommand(writeTypeCommand) == 1 &&
                             g_OptCatalogDamageMaskSlotIndex == 2;

    return extentsOk && originOk && fogRangeOk && fogNearOk && fogStateOk &&
                   rejectOk && writeTypeOk
               ? 0
               : 1;
}

extern "C" int zinterp_context_dispatch_core_resource_globals_smoke()
{
    zInterp_Context context = {};
    zImage_TexDirEntryPartial *oldLensStages[4] = {
        zRndr::g_lensFlareVisibleSampleStages[0],
        zRndr::g_lensFlareVisibleSampleStages[1],
        zRndr::g_lensFlareVisibleSampleStages[2],
        zRndr::g_lensFlareVisibleSampleStages[3],
    };
    const int oldLensFlareVisibilityActive = zRndr::g_lensFlareVisibilityActive;
    const int oldPerspectiveTextureEnabled = zRndr::g_perspectiveTextureEnabled;

    char readCommand[] = "GameZReadZBDFile";
    char emptyPath[] = "";
    char *readTokens[] = {readCommand, emptyPath};
    SetCommandTokens(&context, readTokens, 2);
    const bool readFailureOk = context.DispatchCoreCommand(readCommand) == 1 &&
                               context.lineHadError == 1;
    context.lineHadError = 0;

    char writeCommand[] = "GameZWriteZBDFile";
    char *writeTokens[] = {writeCommand, emptyPath};
    SetCommandTokens(&context, writeTokens, 2);
    const bool writeFailureOk = context.DispatchCoreCommand(writeCommand) == 1 &&
                                context.lineHadError == 0;
    context.lineHadError = 0;

    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 0;
    for (int i = 0; i < 4; ++i) {
        zRndr::g_lensFlareVisibleSampleStages[i] = nullptr;
    }
    zRndr::g_lensFlareVisibilityActive = 0;

    char lensCommand[] = "LensFlareTexture";
    char stageText[] = "2";
    char lensPath[] = "flare.bmp";
    char *lensTokens[] = {lensCommand, stageText, lensPath};
    SetCommandTokens(&context, lensTokens, 3);
    const bool lensOk = context.DispatchCoreCommand(lensCommand) == 1 &&
                        zRndr::g_lensFlareVisibleSampleStages[2] ==
                            &g_zImage_TexDirEntries[0] &&
                        g_zImage_TexDirEntryCount == 1;

    char perspectiveCommand[] = "PerspectiveTexture";
    char onText[] = "on";
    char *perspectiveTokens[] = {perspectiveCommand, onText};
    zRndr::g_perspectiveTextureEnabled = 0;
    SetCommandTokens(&context, perspectiveTokens, 2);
    const bool perspectiveOk = context.DispatchCoreCommand(perspectiveCommand) == 1 &&
                               zRndr::g_perspectiveTextureEnabled == 1;

    char tempDir[MAX_PATH] = {};
    GetTempPathA(sizeof(tempDir), tempDir);
    char setPathText[MAX_PATH] = {};
    char addPathText[MAX_PATH] = {};
    std::snprintf(setPathText, sizeof(setPathText), "%srecoil_zinterp_set_%lu",
                  tempDir, GetCurrentProcessId());
    std::snprintf(addPathText, sizeof(addPathText), "%srecoil_zinterp_add_%lu",
                  tempDir, GetCurrentProcessId());
    CreateDirectoryA(setPathText, nullptr);
    CreateDirectoryA(addPathText, nullptr);

    char rdrSetCommand[] = "RdrSetPath";
    char *rdrSetTokens[] = {rdrSetCommand, setPathText};
    char rdrAddCommand[] = "RdrAddPath";
    char *rdrAddTokens[] = {rdrAddCommand, addPathText};
    if (g_zRdr_SearchPathList != nullptr) {
        zUtil_ZRDR_FreePathList(g_zRdr_SearchPathList);
        std::free(g_zRdr_SearchPathList);
        g_zRdr_SearchPathList = nullptr;
    }
    if (g_zRdr_ScratchSearchPathList != nullptr) {
        zUtil_ZRDR_FreePathList(g_zRdr_ScratchSearchPathList);
        std::free(g_zRdr_ScratchSearchPathList);
        g_zRdr_ScratchSearchPathList = nullptr;
    }
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
    SetCommandTokens(&context, rdrSetTokens, 2);
    const bool rdrSetOk =
        context.DispatchCoreCommand(rdrSetCommand) == 1 && g_zRdr_SearchPathList != nullptr &&
        g_zRdr_SearchPathList->count == 1 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload),
                    setPathText) == 0;
    SetCommandTokens(&context, rdrAddTokens, 2);
    const bool rdrAddOk =
        context.DispatchCoreCommand(rdrAddCommand) == 1 && g_zRdr_SearchPathList != nullptr &&
        g_zRdr_SearchPathList->count == 2 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload),
                    addPathText) == 0 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->next->payload),
                    setPathText) == 0;

    if (g_zRdr_SearchPathList != nullptr) {
        zUtil_ZRDR_FreePathList(g_zRdr_SearchPathList);
        std::free(g_zRdr_SearchPathList);
        g_zRdr_SearchPathList = nullptr;
    }
    if (g_zRdr_ScratchSearchPathList != nullptr) {
        zUtil_ZRDR_FreePathList(g_zRdr_ScratchSearchPathList);
        std::free(g_zRdr_ScratchSearchPathList);
        g_zRdr_ScratchSearchPathList = nullptr;
    }
    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    RemoveDirectoryA(addPathText);
    RemoveDirectoryA(setPathText);
    for (int i = 0; i < 4; ++i) {
        zRndr::g_lensFlareVisibleSampleStages[i] = oldLensStages[i];
    }
    zRndr::g_lensFlareVisibilityActive = oldLensFlareVisibilityActive;
    zRndr::g_perspectiveTextureEnabled = oldPerspectiveTextureEnabled;

    return readFailureOk && writeFailureOk && lensOk && perspectiveOk && rdrSetOk &&
                   rdrAddOk
               ? 0
               : 1;
}

extern "C" int zinterp_context_dispatch_core_object3d_smoke()
{
    zInterp_Context context = {};
    zClass_Object3DDataPartial objectData = {};
    zClass_NodePartial object = {};
    object.classId = 5;
    object.classData = &objectData;
    std::strcpy(object.name, "object");
    context.currentNode = &object;

    char translateCommand[] = "Object3DTranslate";
    char tx[] = "1";
    char ty[] = "2";
    char tz[] = "3";
    char *translateTokens[] = {translateCommand, tx, ty, tz};
    SetCommandTokens(&context, translateTokens, 4);
    const bool translateOk = context.DispatchCoreCommand(translateCommand) == 1 &&
                             objectData.localMatrix[9] == 1.0f &&
                             objectData.localMatrix[10] == 2.0f &&
                             objectData.localMatrix[11] == 3.0f;

    char rotateCommand[] = "Object3DRotate";
    char rx[] = "0";
    char ry[] = "90";
    char rz[] = "180";
    char *rotateTokens[] = {rotateCommand, rx, ry, rz};
    SetCommandTokens(&context, rotateTokens, 4);
    const bool rotateOk = context.DispatchCoreCommand(rotateCommand) == 1 &&
                          objectData.rotation.x == 0.0f &&
                          objectData.rotation.y > 1.5707f &&
                          objectData.rotation.y < 1.5709f &&
                          objectData.rotation.z > 3.1415f &&
                          objectData.rotation.z < 3.1417f;

    char scaleCommand[] = "Object3DScale";
    char sx[] = "7";
    char sy[] = "8";
    char sz[] = "9";
    char *scaleTokens[] = {scaleCommand, sx, sy, sz};
    SetCommandTokens(&context, scaleTokens, 4);
    const bool scaleOk = context.DispatchCoreCommand(scaleCommand) == 1 &&
                         objectData.scale.x == 7.0f &&
                         objectData.scale.y == 8.0f &&
                         objectData.scale.z == 9.0f;

    char activeCommand[] = "Object3DSetActive";
    char onText[] = "on";
    char *activeTokens[] = {activeCommand, onText};
    SetCommandTokens(&context, activeTokens, 2);
    const bool activeOk = context.DispatchCoreCommand(activeCommand) == 1 &&
                          (object.flags & 0x04) != 0;

    char priorityCommand[] = "Object3DSetActionPriority";
    char priorityText[] = "17";
    char *priorityTokens[] = {priorityCommand, priorityText};
    SetCommandTokens(&context, priorityTokens, 2);
    const bool priorityOk = context.DispatchCoreCommand(priorityCommand) == 1 &&
                            object.callbackPriority == 17;

    zModel_MaterialPartial material = {};
    material.packedColor = 0x0034;
    zDiEntryPartial entry = {};
    entry.material = &material;
    zDiPartial di = {};
    di.entryCount = 1;
    di.entries = &entry;
    object.userDataOrDiRef = static_cast<unsigned int>(reinterpret_cast<uintptr_t>(&di));

    char colorCommand[] = "Object3DSetColor";
    char colorModeText[] = "7";
    char *colorTokens[] = {colorCommand, colorModeText};
    SetCommandTokens(&context, colorTokens, 2);
    const bool colorOk = context.DispatchCoreCommand(colorCommand) == 1 &&
                         material.colorRgb.red == 7.0f &&
                         material.colorRgb.green == 0.0f &&
                         material.colorRgb.blue == 0.0f &&
                         material.packedColor == 0x0734 &&
                         material.colorScalar == 1.0f;

    char facadeCommand[] = "Object3DSetFacade";
    char *facadeTokens[] = {facadeCommand};
    di.mode = 0;
    SetCommandTokens(&context, facadeTokens, 1);
    const bool facadeOk = context.DispatchCoreCommand(facadeCommand) == 1 &&
                          di.mode == 1 &&
                          g_zInterp_CurrentCycleTextureDi == &di;

    char opacityCommand[] = "Object3DSetOpacity";
    char opacityText[] = "0.375";
    char *opacityTokens[] = {opacityCommand, opacityText};
    SetCommandTokens(&context, opacityTokens, 2);
    const bool opacityOk = context.DispatchCoreCommand(opacityCommand) == 1 &&
                           objectData.alphaScale > 0.374f &&
                           objectData.alphaScale < 0.376f;

    char opacityIsSetCommand[] = "Object3DSetOpacityIsSet";
    char opacityIsSetText[] = "1";
    char *opacityIsSetTokens[] = {opacityIsSetCommand, opacityIsSetText};
    objectData.flags = 0;
    SetCommandTokens(&context, opacityIsSetTokens, 2);
    const bool opacityIsSetOk = context.DispatchCoreCommand(opacityIsSetCommand) == 1 &&
                                (objectData.flags & 0x02) != 0;

    char pointsCommand[] = "Object3DSetPoints";
    char *pointsTokens[] = {pointsCommand};
    di.mode = 0;
    SetCommandTokens(&context, pointsTokens, 1);
    const bool pointsOk = context.DispatchCoreCommand(pointsCommand) == 1 &&
                          di.mode == 2 &&
                          g_zInterp_CurrentCycleTextureDi == &di;

    char entryPriorityCommand[] = "Object3DSetPriority";
    char entryPriorityText[] = "42";
    char *entryPriorityTokens[] = {entryPriorityCommand, entryPriorityText};
    entry.drawFlags = 0;
    SetCommandTokens(&context, entryPriorityTokens, 2);
    const bool entryPriorityOk = context.DispatchCoreCommand(entryPriorityCommand) == 1 &&
                                 entry.drawFlags == 42;

    char registerCommand[] = "Object3DRegisterTexturesToWorld";
    char *registerTokens[] = {registerCommand, onText};
    di.flags = 0;
    SetCommandTokens(&context, registerTokens, 2);
    const bool registerOk = context.DispatchCoreCommand(registerCommand) == 1 &&
                            (di.flags & 0x04) != 0 &&
                            g_zInterp_CurrentCycleTextureDi == &di;

    char backFaceCommand[] = "Object3DSetShowBackFace";
    char *backFaceTokens[] = {backFaceCommand, onText};
    SetCommandTokens(&context, backFaceTokens, 2);
    const bool backFaceOk = context.DispatchCoreCommand(backFaceCommand) == 1 &&
                            (entry.flagsAndIndexCount & 0x0100u) != 0;

    char baseCommand[] = "Object3DSetTextureWorldBaseCoordinates";
    char baseU[] = "2.25";
    char baseV[] = "3.5";
    char *baseTokens[] = {baseCommand, baseU, baseV};
    SetCommandTokens(&context, baseTokens, 3);
    const bool baseOk = context.DispatchCoreCommand(baseCommand) == 1 &&
                        g_zModel_TextureWorldBaseU == 2.25f &&
                        g_zModel_TextureWorldBaseV == 3.5f;

    char perMeterCommand[] = "Object3DSetTextureWorldTexturesPerMeter";
    char perMeterU[] = "4.5";
    char perMeterV[] = "6.75";
    char *perMeterTokens[] = {perMeterCommand, perMeterU, perMeterV};
    SetCommandTokens(&context, perMeterTokens, 3);
    const bool perMeterOk = context.DispatchCoreCommand(perMeterCommand) == 1 &&
                            g_zModel_TextureWorldPerMeterU == 4.5f &&
                            g_zModel_TextureWorldPerMeterV == 6.75f;

    return translateOk && rotateOk && scaleOk && activeOk && priorityOk && colorOk &&
                   facadeOk && opacityOk && opacityIsSetOk && pointsOk && entryPriorityOk &&
                   registerOk && backFaceOk && baseOk && perMeterOk
               ? 0
               : 1;
}

extern "C" int zinterp_context_dispatch_core_model_material_smoke()
{
    zInterp_Context context = {};
    zInterp_RuntimeBlob runtime = {};
    context.runtimeBlob = &runtime;

    zImage_TexDirEntryPartial oldEntry = g_zImage_TexDirEntries[0];
    const int oldEntryCount = g_zImage_TexDirEntryCount;
    g_zImage_TexDirEntryCount = 1;
    std::memset(&g_zImage_TexDirEntries[0], 0, sizeof(g_zImage_TexDirEntries[0]));
    std::strcpy(g_zImage_TexDirEntries[0].baseName, "panel");
    g_zImage_TexDirEntries[0].loadState = 1;

    char matlNewCommand[] = "MatlNew";
    char *matlNewTokens[] = {matlNewCommand};
    SetCommandTokens(&context, matlNewTokens, 1);
    const bool matlNewOk = context.DispatchCoreCommand(matlNewCommand) == 1 &&
                           runtime.material.flags == 0x00ff &&
                           runtime.material.packedColor == 0x7fff &&
                           runtime.material.colorScalar == 0.5f;

    char colorCommand[] = "MatlFaceColor";
    char redText[] = "128";
    char greenText[] = "64";
    char blueText[] = "32";
    char *colorTokens[] = {colorCommand, redText, greenText, blueText};
    SetCommandTokens(&context, colorTokens, 4);
    const bool colorOk = context.DispatchCoreCommand(colorCommand) == 1 &&
                         runtime.material.colorRgb.red == 128.0f &&
                         runtime.material.colorRgb.green == 64.0f &&
                         runtime.material.colorRgb.blue == 32.0f &&
                         runtime.material.packedColor == zVid_PackColorRgbFloats(
                                                              (zVideo_ColorRgbFloat
                                                                   *)(&runtime.material.colorRgb));

    char textureCommand[] = "MatlTexture";
    char textureName[] = "panel";
    char *textureTokens[] = {textureCommand, textureName};
    runtime.material.flags = 0;
    SetCommandTokens(&context, textureTokens, 2);
    const bool textureOk = context.DispatchCoreCommand(textureCommand) == 1 &&
                           runtime.material.currentTextureDirectoryEntry ==
                               &g_zImage_TexDirEntries[0] &&
                           (runtime.material.flags & 0x0100) != 0;

    zDiPartial di = {};
    runtime.displayInstance = &di;
    zModel_MaterialPartial *const oldReuseCache = g_zModel_MatlReuseCache;
    g_zModel_MatlReuseCache = &runtime.material;

    char beginCommand[] = "ModelPolygonBegin";
    char *beginTokens[] = {beginCommand};
    runtime.pointCount = 7;
    runtime.uvCount = 5;
    runtime.normalsA = reinterpret_cast<zVec3 *>(1);
    SetCommandTokens(&context, beginTokens, 1);
    const bool beginOk = context.DispatchCoreCommand(beginCommand) == 1 &&
                         runtime.pointCount == 0 && runtime.uvCount == 0 &&
                         runtime.normalsA == nullptr;

    char vertexCommand[] = "ModelPolygonVertex";
    char vx0[] = "0";
    char vy0[] = "0";
    char vz0[] = "0";
    char vx1[] = "1";
    char vy1[] = "0";
    char vz1[] = "0";
    char vx2[] = "0";
    char vy2[] = "1";
    char vz2[] = "0";
    char *vertex0Tokens[] = {vertexCommand, vx0, vy0, vz0};
    char *vertex1Tokens[] = {vertexCommand, vx1, vy1, vz1};
    char *vertex2Tokens[] = {vertexCommand, vx2, vy2, vz2};
    SetCommandTokens(&context, vertex0Tokens, 4);
    const bool vertex0Ok = context.DispatchCoreCommand(vertexCommand) == 1;
    SetCommandTokens(&context, vertex1Tokens, 4);
    const bool vertex1Ok = context.DispatchCoreCommand(vertexCommand) == 1;
    SetCommandTokens(&context, vertex2Tokens, 4);
    const bool vertex2Ok = context.DispatchCoreCommand(vertexCommand) == 1 &&
                           runtime.pointCount == 3 &&
                           runtime.polygonPoints[2].y == 1.0f;

    char uvCommand[] = "ModelPolygonUV";
    char u0[] = "4";
    char v0[] = "5";
    char u1[] = "5";
    char v1[] = "5";
    char u2[] = "4";
    char v2[] = "6";
    char *uv0Tokens[] = {uvCommand, u0, v0};
    char *uv1Tokens[] = {uvCommand, u1, v1};
    char *uv2Tokens[] = {uvCommand, u2, v2};
    SetCommandTokens(&context, uv0Tokens, 3);
    const bool uv0Ok = context.DispatchCoreCommand(uvCommand) == 1;
    SetCommandTokens(&context, uv1Tokens, 3);
    const bool uv1Ok = context.DispatchCoreCommand(uvCommand) == 1;
    SetCommandTokens(&context, uv2Tokens, 3);
    const bool uv2Ok = context.DispatchCoreCommand(uvCommand) == 1 &&
                       runtime.uvCount == 3 && runtime.uvPairs[2].v == 6.0f;

    char endCommand[] = "ModelPolygonEnd";
    char *endTokens[] = {endCommand};
    runtime.drawFlags = 0x44;
    runtime.flagBit8 = 1;
    SetCommandTokens(&context, endTokens, 1);
    const bool endOk = context.DispatchCoreCommand(endCommand) == 1 &&
                       runtime.polygonMaterial == &runtime.material &&
                       runtime.variantTag.count == 0 &&
                       di.entryCount == 1 && di.entries != nullptr &&
                       di.entries[0].drawFlags == 0x44 &&
                       (di.entries[0].flagsAndIndexCount & 0x0100u) != 0 &&
                       di.entries[0].material == &runtime.material;

    if (di.entries != nullptr) {
        std::free(di.entries[0].vertexIndices);
        std::free(di.entries[0].normalIndices);
        std::free(di.entries[0].uvPairs);
    }
    std::free(di.entries);
    std::free(di.verts);
    std::free(di.normals);
    std::free(di.blendVerts);

    g_zModel_MatlReuseCache = oldReuseCache;
    g_zImage_TexDirEntries[0] = oldEntry;
    g_zImage_TexDirEntryCount = oldEntryCount;

    return matlNewOk && colorOk && textureOk && beginOk && vertex0Ok && vertex1Ok &&
                   vertex2Ok && uv0Ok && uv1Ok && uv2Ok && endOk
               ? 0
               : 1;
}

extern "C" int zinterp_context_dispatch_core_light_lod_smoke()
{
    zInterp_Context context = {};
    zClass_LightDataPartial lightData = {};
    zClass_NodePartial light = {};
    light.classId = 9;
    light.classData = &lightData;
    context.currentNode = &light;

    char lightActiveCommand[] = "LightSetActive";
    char onText[] = "on";
    char *lightActiveTokens[] = {lightActiveCommand, onText};
    SetCommandTokens(&context, lightActiveTokens, 2);
    const bool lightActiveOk = context.DispatchCoreCommand(lightActiveCommand) == 1 &&
                               (light.flags & 0x04) != 0;

    char ambientCommand[] = "LightSetAmbient";
    char ambientText[] = "0.25";
    char *ambientTokens[] = {ambientCommand, ambientText};
    lightData.dirty = 0;
    SetCommandTokens(&context, ambientTokens, 2);
    const bool ambientOk = context.DispatchCoreCommand(ambientCommand) == 1 &&
                           lightData.intensityScale == 0.25f &&
                           lightData.dirty == 1;

    char colorCommand[] = "LightSetColor";
    char redText[] = "0.125";
    char greenText[] = "0.5";
    char blueText[] = "0.75";
    char *colorTokens[] = {colorCommand, redText, greenText, blueText};
    lightData.dirty = 0;
    SetCommandTokens(&context, colorTokens, 4);
    const bool colorOk = context.DispatchCoreCommand(colorCommand) == 1 &&
                         lightData.specularColor.red == 0.125f &&
                         lightData.specularColor.green == 0.5f &&
                         lightData.specularColor.blue == 0.75f &&
                         lightData.dirty == 1;

    char diffuseCommand[] = "LightSetDiffuse";
    char diffuseText[] = "0.75";
    char *diffuseTokens[] = {diffuseCommand, diffuseText};
    lightData.dirty = 0;
    SetCommandTokens(&context, diffuseTokens, 2);
    const bool diffuseOk = context.DispatchCoreCommand(diffuseCommand) == 1 &&
                           lightData.falloff == 0.75f &&
                           lightData.dirty == 1;

    char translateCommand[] = "LightSetTranslate";
    char lx[] = "1";
    char ly[] = "2";
    char lz[] = "3";
    char *translateTokens[] = {translateCommand, lx, ly, lz};
    SetCommandTokens(&context, translateTokens, 4);
    const bool translateOk = context.DispatchCoreCommand(translateCommand) == 1 &&
                             lightData.localPosition.x == 1.0f &&
                             lightData.localPosition.y == 2.0f &&
                             lightData.localPosition.z == 3.0f &&
                             lightData.dirty == 1;

    char orientCommand[] = "LightSetOrientation";
    char rx[] = "0";
    char ry[] = "90";
    char rz[] = "180";
    char *orientTokens[] = {orientCommand, rx, ry, rz};
    lightData.dirty = 0;
    SetCommandTokens(&context, orientTokens, 4);
    const bool orientOk = context.DispatchCoreCommand(orientCommand) == 1 &&
                          lightData.localRotation.x == 0.0f &&
                          lightData.localRotation.y > 1.5707f &&
                          lightData.localRotation.y < 1.5709f &&
                          lightData.localRotation.z > 3.1415f &&
                          lightData.localRotation.z < 3.1417f &&
                          lightData.dirty == 1;

    char rangesCommand[] = "LightSetRanges";
    char nearText[] = "20";
    char farText[] = "10";
    char *rangeTokens[] = {rangesCommand, nearText, farText};
    lightData.dirty = 0;
    SetCommandTokens(&context, rangeTokens, 3);
    const bool rangesOk = context.DispatchCoreCommand(rangesCommand) == 1 &&
                          lightData.range1 == 10.0f &&
                          lightData.range2 == 20.0f &&
                          lightData.range2Sq == 400.0f &&
                          lightData.dirty == 1;

    char pointCommand[] = "LightSetPointSource";
    char *pointTokens[] = {pointCommand};
    lightData.dirty = 0;
    lightData.isPointMode = 1;
    lightData.isDirectionalMode = 0;
    SetCommandTokens(&context, pointTokens, 1);
    const bool pointOk = context.DispatchCoreCommand(pointCommand) == 1 &&
                         lightData.isPointMode == 0 &&
                         lightData.isDirectionalMode == 1 &&
                         lightData.dirty == 1;

    char directedCommand[] = "LightSetDirectedSource";
    char *directedTokens[] = {directedCommand};
    lightData.dirty = 0;
    SetCommandTokens(&context, directedTokens, 1);
    const bool directedOk = context.DispatchCoreCommand(directedCommand) == 1 &&
                            lightData.isPointMode == 1 &&
                            lightData.isDirectionalMode == 0 &&
                            lightData.dirty == 1;

    char directionalCommand[] = "LightSetDirectional";
    char *directionalTokens[] = {directionalCommand, onText};
    lightData.dirty = 0;
    SetCommandTokens(&context, directionalTokens, 2);
    unsigned int coneBits = 0;
    const bool directionalDispatchOk = context.DispatchCoreCommand(directionalCommand) == 1;
    std::memcpy(&coneBits, &lightData.coneAngle, sizeof(coneBits));
    const bool directionalOk = directionalDispatchOk && coneBits == 1 &&
                               lightData.dirty == 1;

    char saturatedCommand[] = "LightSetSaturated";
    char *saturatedTokens[] = {saturatedCommand, onText};
    lightData.dirty = 0;
    SetCommandTokens(&context, saturatedTokens, 2);
    const bool saturatedOk = context.DispatchCoreCommand(saturatedCommand) == 1 &&
                             lightData.lightParam == 1 &&
                             lightData.dirty == 1;

    zClass_LodDataPartial lodData = {};
    zClass_NodePartial lod = {};
    lod.classId = 6;
    lod.classData = &lodData;
    context.currentNode = &lod;

    char rangeCommand[] = "LODSetRange";
    char nearRangeText[] = "3";
    char farRangeText[] = "7";
    char *lodRangeTokens[] = {rangeCommand, nearRangeText, farRangeText};
    SetCommandTokens(&context, lodRangeTokens, 3);
    const bool lodRangeOk = context.DispatchCoreCommand(rangeCommand) == 1 &&
                            lodData.nearRangeSq == 9.0f &&
                            lodData.farRangeSq == 49.0f;

    return lightActiveOk && ambientOk && colorOk && diffuseOk && translateOk && orientOk &&
                   rangesOk && pointOk && directedOk && directionalOk && saturatedOk &&
                   lodRangeOk
               ? 0
               : 1;
}
