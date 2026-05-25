#include "GameZRecoil/zInterp/zInterp.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zVideo/zVideo.h"

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
    context.runtimeBlob = std::malloc(4);
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
        static_cast<const unsigned char *>(context.runtimeBlob);
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
