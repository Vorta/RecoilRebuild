#include "GameZRecoil/zInterp/zInterp.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zImage.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "Battlesport/zUtil/zutil.h"

#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

namespace
{
const int kPreparedScriptMagic = 0x08971119;
const int kPreparedScriptVersion = 7;
const double kDegreesToRadians = 0.01745329251994;
const char kTokenDelimiters[] = ", \t\n";
char g_zInterp_MacroExpansionScratch[2048];
} // namespace

const int g_zInterp_Context_VTableMarker = 0;
int g_zInterp_EnablePreparedScripts = 0;
int g_zInterp_VerboseLevel = 0;
char g_zInterp_LineBuffer[1024] = {0};
char g_zInterp_AssignToken_Equal = '=';
unsigned int g_zInterp_NodeUserDataScratch = 0;
zDiPartial *g_zInterp_CurrentCycleTextureDi = 0;
zInterp_Context g_zInterp_GlobalContext = {0};

namespace
{
int RECOIL_CDECL DefaultPreDispatch(zInterp_Context *, char *)
{
    return 1;
}

int RECOIL_CDECL DefaultPostDispatch(zInterp_Context *, char *)
{
    return 0;
}

int RECOIL_CDECL DefaultDeferredDispatch(zInterp_Context *, char *)
{
    return 0;
}

const zInterp_Context_VTable g_zInterp_DefaultRuntimeVTable = {
    DefaultPreDispatch,
    DefaultPostDispatch,
    DefaultDeferredDispatch,
};

const zInterp_Context_VTable *RuntimeVTable(const zInterp_Context *ctx)
{
    if (ctx->vftable == 0 || ctx->vftable == &g_zInterp_Context_VTableMarker) {
        return &g_zInterp_DefaultRuntimeVTable;
    }
    return static_cast<const zInterp_Context_VTable *>(ctx->vftable);
}

char *CurrentCommandToken(zInterp_Context *ctx)
{
    return ctx->tokenCount > 0 ? ctx->tokenList[0] : 0;
}

int CommandIs(zInterp_Context *ctx, const char *text)
{
    return ctx->CommandEqualsPrefix(text, static_cast<unsigned int>(strlen(text)));
}

int CommandIsExact(zInterp_Context *ctx, const char *text)
{
    return ctx->CommandEquals(text);
}

int CommandHasPrefix(zInterp_Context *ctx, const char *text)
{
    return ctx->CommandEqualsPrefix(text, static_cast<unsigned int>(strlen(text)));
}
} // namespace

// Reimplements 0x4c58c0: zInterp_Context::DefaultDispatchHook
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
zInterp_Context::DefaultDispatchHook(zClass_NodePartial *node)
{
    if (node != 0) {
        zClass_Class::gwNodeGetUserData(node, 0);
    }

    return 0;
}

namespace zInterp_Object3D
{
// Reimplements 0x4c59e0: zInterp_Object3D::DefaultRenderAction
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL DefaultRenderAction(zClass_NodePartial *node)
{
    unsigned int userData = 0;
    zClass_Class::gwNodeGetUserData(node, &userData);
    return zModel_Instance_UpdateScrollingTexturesIfNeeded(
        (zModel_InstancePartial *)(userData));
}

// Reimplements 0x4c5a00: zInterp_Object3D::ScrollAlwaysTickAction
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL ScrollAlwaysTickAction(zClass_NodePartial *wrapperNode)
{
    if (wrapperNode == 0) {
        return;
    }

    zInterp_Context *const context = (zInterp_Context *)(wrapperNode->callbackContext);
    zInterp_LinkNode *const head = context->scrollAlwaysListHead;
    zInterp_LinkNode *entry = head->next;
    while (entry != head) {
        DefaultRenderAction((zClass_NodePartial *)(entry->payload));
        entry = entry->next;
    }
}
} // namespace zInterp_Object3D

// Reimplements 0x4c1b30: zInterp_Context::Logf
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_CDECL
zInterp_Context::Logf(zInterp_Context *ctx, const char *fmt, ...)
{
    if (ctx->logFn != 0) {
        va_list args;
        va_start(args, fmt);
        ctx->logFn(fmt, (char *)args);
        va_end(args);
    }
}

// Reimplements 0x4c5520: zInterp_Context::ReportErrorf
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_CDECL
zInterp_Context::ReportErrorf(zInterp_Context *ctx, const char *fmt, ...)
{
    ctx->lineHadError = 1;
    if (ctx->logFn != 0) {
        va_list args;
        va_start(args, fmt);
        ctx->logFn(fmt, (char *)args);
        va_end(args);
    }
}

// Reimplements 0x4c1b20: zInterp_Context::IncErrorCount
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::IncErrorCount()
{
    ++errorCount;
}

// Reimplements 0x4c15f0: zInterp_Context::FindMacroValue
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE char *RECOIL_THISCALL
zInterp_Context::FindMacroValue(const char *name, zInterp_MacroEntry **outEntry)
{
    for (unsigned int macroIndex = 0; macroIndex < macroCount; ++macroIndex) {
        zInterp_MacroEntry *const entry = &macroTable[macroIndex];
        if (strcmp(entry->name, name) == 0) {
            if (outEntry != 0) {
                *outEntry = entry;
            }
            return entry->value;
        }
    }

    return 0;
}

// Reimplements 0x4c1710: zInterp_Context::IsMacroTrue
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::IsMacroTrue(const char *name)
{
    const char *const value = FindMacroValue(name, 0);
    return value != 0 && strcmp(value, "TRUE") == 0;
}

// Reimplements 0x4c1780: zInterp_Context::SetMacro
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::SetMacro(const char *name, const char *value)
{
    zInterp_MacroEntry *entry = 0;
    if (name == 0 || value == 0) {
        return 0;
    }

    if (FindMacroValue(name, &entry) != 0) {
        const size_t valueSize = strlen(value) + 1;
        entry->value = static_cast<char *>(realloc(entry->value, valueSize));
        memcpy(entry->value, value, valueSize);
        return 1;
    }

    macroTable = static_cast<zInterp_MacroEntry *>(
        realloc(macroTable, (macroCount + 1) * sizeof(zInterp_MacroEntry)));
    entry = &macroTable[macroCount];
    entry->name = _strdup(name);
    entry->value = _strdup(value);
    ++macroCount;
    return 1;
}

// Reimplements 0x4c1670: zInterp_Context::ClearMacroTable
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::ClearMacroTable()
{
    for (unsigned int macroIndex = 0; macroIndex < macroCount; ++macroIndex) {
        free(macroTable[macroIndex].name);
        free(macroTable[macroIndex].value);
    }

    if (macroTable != 0) {
        free(macroTable);
    }
    macroTable = 0;
    macroCount = 0;
}

// Reimplements 0x4c16c0: zInterp_Context::ClearVarTable
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::ClearVarTable()
{
    for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex) {
        free(varTable[varIndex].name);
    }

    if (varTable != 0) {
        free(varTable);
    }
    varTable = 0;
    varCount = 0;
}

// Reimplements 0x4c0d20: zInterp_Context::Constructor
// (D:\Proj\GameZRecoil\zInterp\interp_context.c)
RECOIL_NOINLINE zInterp_Context *RECOIL_THISCALL
zInterp_Context::Constructor(const char *searchPathText, const char *preparedIndexPath)
{
    searchPathLeadChar = searchPathText[0];

    zInterp_LinkNode *const head = new zInterp_LinkNode;
    head->next = head;
    head->prev = head;
    scrollAlwaysListHead = head;
    scrollAlwaysListCount = 0;

    vftable = &g_zInterp_Context_VTableMarker;
    includeDepth = 0;

    runtimeBlob = static_cast<zInterp_RuntimeBlob *>(malloc(sizeof(zInterp_RuntimeBlob)));
    memset(runtimeBlob, 0, 0x1d8);

    currentNode = 0;
    conditionalDepth = 0;
    tempAlloc = 0;
    hasPreparedInput = 0;
    tokenCount = 0;
    logFn = 0;
    unknown_04 = 0;
    scrollAlwaysDriverNode = 0;

    searchPathSpec = _strdup(searchPathText);
    preparedIndexFileName = _strdup(preparedIndexPath);

    archiveSearchList = 0;
    currentScriptFile = 0;
    fileFrameCount = 0;
    fileFrameStack = 0;
    preparedIndexStream = 0;
    preparedIndexMagic = 0;
    preparedIndexVersion = 0;

    preparedEntryCount = static_cast<int *>(malloc(sizeof(int)));
    *preparedEntryCount = 0;

    preparedEntryTable = 0;
    macroTable = 0;
    macroCount = 0;
    varTable = 0;
    varCount = 0;
    ptrArrayHead = 0;
    ptrArrayCount = 0;

    return this;
}

// Reimplements 0x4c0f70: zInterp_Context::Destroy
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::Destroy()
{
    ClearMacroTable();
    ClearVarTable();
    ClearFileFrameStack();

    if (archiveSearchList != 0) {
        zUtil_ZRDR_FreeSearchPathList(archiveSearchList);
        archiveSearchList = 0;
    }

    scrollAlwaysDriverNode = 0;

    zInterp_LinkNode *const head = scrollAlwaysListHead;
    zInterp_LinkNode *node = head->next;
    while (node != head) {
        zInterp_LinkNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        delete node;
        --scrollAlwaysListCount;
        node = next;
    }

    if (ptrArrayHead != 0) {
        free(ptrArrayHead);
    }
    ptrArrayHead = 0;
    ptrArrayCount = 0;
    includeDepth = 0;
}

// Reimplements 0x4c0e50: zInterp_Context::Destructor
// (D:\Proj\GameZRecoil\zInterp\interp_context.c)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::Destructor()
{
    vftable = &g_zInterp_Context_VTableMarker;

    Destroy();

    if (tempAlloc != 0) {
        free(tempAlloc);
    }
    if (runtimeBlob != 0) {
        free(runtimeBlob);
    }
    if (preparedEntryCount != 0) {
        free(preparedEntryCount);
    }
    if (preparedEntryTable != 0) {
        free(preparedEntryTable);
    }
    if (preparedIndexFileName != 0) {
        free(preparedIndexFileName);
    }
    if (searchPathSpec != 0) {
        free(searchPathSpec);
    }

    zInterp_LinkNode *const head = scrollAlwaysListHead;
    zInterp_LinkNode *node = head->next;
    while (node != head) {
        zInterp_LinkNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --scrollAlwaysListCount;
        node = next;
    }

    ::operator delete(head);
    scrollAlwaysListHead = 0;
    scrollAlwaysListCount = 0;
}

// Reimplements 0x4c58e0: zInterp_Context::RegisterScrollAlwaysNode
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::RegisterScrollAlwaysNode(
    zClass_NodePartial *node, float textureWorldPerMeter, int textureWorldAxis,
    int installDriverCallback)
{
    if (node == 0) {
        return 0;
    }

    unsigned int diValue = 0;
    zClass_Class::gwNodeGetUserData(node, &diValue);
    zDiPartial *const di = (zDiPartial *)(diValue);
    if (di == 0) {
        return 0;
    }

    zModel::SetDiTextureWorldPerMeter(di, 1, textureWorldPerMeter, textureWorldAxis);

    if (installDriverCallback == 0) {
        zClass_Class::gwNodeSetActionCallback(
            node, (void *)(&zInterp_Object3D::DefaultRenderAction));
        return 1;
    }

    if (scrollAlwaysDriverNode == 0) {
        scrollAlwaysDriverNode = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetActionCallback(
            scrollAlwaysDriverNode, (void *)(&zInterp_Object3D::ScrollAlwaysTickAction));
        zClass_Class::gwNodeSetName(scrollAlwaysDriverNode, "ScrollAlways");
        scrollAlwaysDriverNode->callbackContext = (zClass_NodePartial *)(this);
    }

    zInterp_LinkNode *const head = scrollAlwaysListHead;
    zInterp_LinkNode *const tail = head->prev;
    zInterp_LinkNode *const entry = new zInterp_LinkNode;
    entry->next = head != 0 ? head : entry;
    entry->prev = tail != 0 ? tail : entry;
    head->prev = entry;
    entry->prev->next = entry;
    entry->payload = node;
    ++scrollAlwaysListCount;
    return 1;
}

// Reimplements 0x4c1b50: zInterp_Context::EvalConditionExpr
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::EvalConditionExpr()
{
    if (tokenCount == 1) {
        return 0;
    }

    if (tokenCount == 2) {
        return IsMacroTrue(tokenList[1]) != 0;
    }

    unsigned int tokenIndex = 1;
    int op = 0;
    int result = 0;
    while (tokenIndex < tokenCount) {
        const char *const name = tokenList[tokenIndex];
        ++tokenIndex;

        if (op == 0) {
            result = IsMacroTrue(name);
        } else if (op == 1) {
            result |= IsMacroTrue(name);
        } else if (op == 2) {
            result &= IsMacroTrue(name);
        }

        if (tokenIndex < tokenCount) {
            const char *const opText = tokenList[tokenIndex];
            ++tokenIndex;
            if (strncmp(opText, "||", 2) == 0) {
                op = 1;
            } else if (strncmp(opText, "&&", 2) == 0) {
                op = 2;
            } else {
                break;
            }
        }
    }

    return result;
}

// Reimplements 0x4c1250: zInterp_Context::ExpandMacroRefs
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE char *RECOIL_THISCALL zInterp_Context::ExpandMacroRefs(char *lineBuf)
{
    if (lineBuf == 0) {
        return 0;
    }

    char *open = strchr(lineBuf, '%');
    char *close = open != 0 ? strchr(open + 1, '%') : 0;
    if (open == 0 || close == 0) {
        return lineBuf;
    }

    g_zInterp_MacroExpansionScratch[0] = '\0';
    char *segmentStart = lineBuf;
    char macroName[64];

    while (open != 0 && close != 0) {
        const int literalLength = static_cast<int>(open - segmentStart);
        if (literalLength != 0) {
            strncat(g_zInterp_MacroExpansionScratch, segmentStart, literalLength);
        }

        const int macroSpanLength = static_cast<int>(close - open);
        strncpy(macroName, open + 1, macroSpanLength);
        macroName[macroSpanLength - 1] = '\0';

        char *const value = FindMacroValue(macroName, 0);
        if (value != 0) {
            strcat(g_zInterp_MacroExpansionScratch, value);
        }

        segmentStart = close + 1;
        open = strchr(segmentStart, '%');
        close = open != 0 ? strchr(open + 1, '%') : 0;
    }

    if (segmentStart != 0 && *segmentStart != '\0') {
        strcat(g_zInterp_MacroExpansionScratch, segmentStart);
    }

    return g_zInterp_MacroExpansionScratch;
}

// Reimplements 0x4c1990: zInterp_Context::NextToken
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE char *RECOIL_THISCALL zInterp_Context::NextToken()
{
    const unsigned int tokenIndex = static_cast<unsigned int>(tokenReadIndex);
    tokenReadIndex = static_cast<int>(tokenIndex + 1);

    char *token = 0;
    if (tokenIndex < tokenCount) {
        token = tokenList[tokenIndex];
    }

    if (token == 0) {
        return 0;
    }

    return ExpandMacroRefs(token);
}

// Reimplements 0x4c19c0: zInterp_Context::ParseBoolToken
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::ParseBoolToken()
{
    char *const token = NextToken();
    if (token == 0) {
        return 0;
    }

    return _stricmp(token, "on") == 0 || _stricmp(token, "true") == 0;
}

// Reimplements 0x4c1a00: zInterp_Context::ParseFloatToken
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE float RECOIL_THISCALL zInterp_Context::ParseFloatToken()
{
    char *const token = NextToken();
    if (token == 0) {
        return 0.0f;
    }

    return static_cast<float>(atof(token));
}

// Reimplements 0x4c1a20: zInterp_Context::ParseIntToken
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::ParseIntToken()
{
    char *const token = NextToken();
    if (token == 0) {
        return 0;
    }

    return atoi(token);
}

// Reimplements 0x4c1a40: zInterp_Context::FindVarEntry
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE zInterp_VarEntry *RECOIL_THISCALL
zInterp_Context::FindVarEntry(const char *name)
{
    for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex) {
        zInterp_VarEntry *const entry = &varTable[varIndex];
        if (strcmp(entry->name, name) == 0) {
            return entry;
        }
    }

    return 0;
}

// Reimplements 0x4c1ab0: zInterp_Context::DumpVarEntry
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::DumpVarEntry(zInterp_VarEntry *entry)
{
    if (entry == 0) {
        return;
    }

    if (entry->type == 0) {
        Logf(this, "%s = (int) %d", entry->name, *entry->valuePtr.intPtr);
        return;
    }

    if (entry->type == 1) {
        Logf(this, "%s = (float) %f", entry->name, *entry->valuePtr.floatPtr);
        return;
    }

    if (entry->type == 2) {
        // Original code passes *charPtr to a %s format, so varargs receives an int,
        // not the string pointer. Keep that source-shape bug visible for the cluster pass.
        Logf(this, "%s = (char*)\"%s\"", entry->name, *entry->valuePtr.charPtr);
    }
}

// Reimplements 0x4c5480: zInterp_Context::CommandEqualsPrefix
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::CommandEqualsPrefix(const char *prefix, unsigned int prefixLen)
{
    char *command = 0;
    if (tokenCount > 0) {
        command = tokenList[0];
    }

    return strncmp(command, prefix, prefixLen) == 0;
}

// Reimplements 0x4c54b0: zInterp_Context::CommandEquals
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::CommandEquals(const char *other)
{
    char *command = 0;
    if (tokenCount > 0) {
        command = tokenList[0];
    }

    return strcmp(command, other) == 0;
}

// Reimplements 0x4c5510: zInterp_Context::GetCurrentCommand
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE char *RECOIL_THISCALL zInterp_Context::GetCurrentCommand()
{
    if (tokenCount <= 0) {
        return 0;
    }

    return tokenList[0];
}

// Reimplements 0x4c5820: zInterp_Context::ValidateArgsAndNodeType
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::ValidateArgsAndNodeType(int expectedArgCount, int expectedClassType,
                                         zClass_NodePartial *node)
{
    if (expectedClassType != 0) {
        if (node == 0) {
            ReportErrorf(this, "Interp: keyword [%s] has NULL node to work with",
                         tokenCount > 0 ? tokenList[0] : 0);
            return 0;
        }

        const int classType = node->classId;
        if (classType != expectedClassType) {
            ReportErrorf(this,
                         "Interp: keyword [%s] has node [%s] of class=%d, expected=%d",
                         tokenCount > 0 ? tokenList[0] : 0, node, classType,
                         expectedClassType);
            return 0;
        }
    }

    const int currentTokenCount = tokenCount;
    if (static_cast<unsigned int>(expectedArgCount + 1) <=
        static_cast<unsigned int>(currentTokenCount)) {
        return 1;
    }

    ReportErrorf(this, "Interp: keyword [%s] requires (%d) args, found (%d) args",
                 currentTokenCount > 0 ? tokenList[0] : 0, expectedArgCount,
                 currentTokenCount - 1);
    return 0;
}

// Reimplements 0x4c5550: zInterp_Context::LoadPreparedScriptIndex
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::LoadPreparedScriptIndex(const char *zrdrPath)
{
    int preparedMagic = 0;
    int preparedVersion = 0;
    unsigned int preparedEntryCountValue = 0;
    int preparedHeader[2] = {0, 0};

    if (preparedIndexStream != 0) {
        return 1;
    }

    if (archiveSearchList == 0) {
        archiveSearchList = zUtil_ZRDR_CreateSearchPathList(searchPathSpec);
    }

    preparedIndexStream = zUtil_ZRDR_OpenFileResolved(archiveSearchList, zrdrPath, "rb");
    if (preparedIndexStream == 0) {
        return 0;
    }

    if (fread(preparedHeader, sizeof(preparedHeader), 1, preparedIndexStream) != 1) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
        return 0;
    }
    preparedMagic = preparedHeader[0];
    preparedVersion = preparedHeader[1];

    if (preparedMagic != kPreparedScriptMagic) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
        return 0;
    }

    if (preparedVersion != kPreparedScriptVersion ||
        fread(&preparedEntryCountValue, 4, 1, preparedIndexStream) != 1) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
        return 0;
    }

    zInterp_PreparedScriptEntry *entries = static_cast<zInterp_PreparedScriptEntry *>(
        realloc(0, (preparedEntryCountValue + 1) * sizeof(zInterp_PreparedScriptEntry)));
    if (entries == 0) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
        return 0;
    }

    if (fread(entries, sizeof(zInterp_PreparedScriptEntry), preparedEntryCountValue,
              preparedIndexStream) != preparedEntryCountValue) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
        return 0;
    }

    int entriesFresh = 1;
    for (unsigned int entryIndex = 0; entriesFresh != 0 && entryIndex < preparedEntryCountValue;
         ++entryIndex) {
        struct _stat sourceStat;
        if (_stat(entries[entryIndex].path, &sourceStat) == 0 &&
            entries[entryIndex].fileTime != sourceStat.st_mtime) {
            entriesFresh = 0;
        }
    }

    if (entriesFresh != 0) {
        preparedIndexMagic = preparedMagic;
        preparedIndexVersion = preparedVersion;
        *preparedEntryCount = static_cast<int>(preparedEntryCountValue);
        preparedEntryTable = entries;
        return 1;
    }

    fclose(preparedIndexStream);
    preparedIndexStream = 0;
    free(entries);
    return 0;
}

// Reimplements 0x4c5740: zInterp_Context::OpenPreparedScriptStream
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE FILE *RECOIL_THISCALL
zInterp_Context::OpenPreparedScriptStream(const char *commandName)
{
    if (preparedIndexStream == 0) {
        return 0;
    }

    int matchedIndex = -1;
    for (int entryIndex = 0; entryIndex < *preparedEntryCount; ++entryIndex) {
        if (_stricmp(preparedEntryTable[entryIndex].path, commandName) == 0) {
            matchedIndex = entryIndex;
            break;
        }
    }

    if (matchedIndex == -1) {
        return 0;
    }

    zInterp_PreparedScriptEntry *const matchedEntry = &preparedEntryTable[matchedIndex];
    int usePreparedStream = 1;
    struct _stat sourceStat;
    if (_stat(commandName, &sourceStat) == 0 &&
        difftime(sourceStat.st_mtime, matchedEntry->fileTime) > 0.0) {
        usePreparedStream = 0;
    }

    if (usePreparedStream == 0) {
        return 0;
    }

    FILE *const stream = preparedIndexStream;
    if (fseek(stream, matchedEntry->fileOffset, SEEK_SET) == 0) {
        return stream;
    }

    return 0;
}

// Reimplements 0x4c1500: zInterp_Context::RunScriptFile
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::RunScriptFile(const char *filePath)
{
    FILE *scriptFile = 0;
    int hasPrepared = 0;

    if (currentScriptFile != 0) {
        const long filePos = ftell(currentScriptFile);
        PushFileFrame(currentScriptFile, filePos, hasPreparedInput);
    }

    if (g_zInterp_EnablePreparedScripts != 0 &&
        LoadPreparedScriptIndex(preparedIndexFileName) != 0) {
        scriptFile = OpenPreparedScriptStream(filePath);
        if (scriptFile != 0) {
            hasPrepared = 1;
        }
    }

    if (scriptFile == 0) {
        scriptFile = fopen(filePath, "r");
        ++includeDepth;
    }

    int result = 0;
    if (scriptFile != 0) {
        result = RunString(scriptFile, hasPrepared);
        if (hasPrepared == 0) {
            fclose(scriptFile);
        }
    }

    zInterp_FileFrame *const frame = PopFileFrame();
    if (frame != 0) {
        currentScriptFile = frame->file;
        fseek(frame->file, frame->filePos, 0);
        hasPreparedInput = frame->hasPreparedInput;
        return result;
    }

    currentScriptFile = 0;
    return result;
}

// Reimplements 0x4c1020: zInterp_Context::RunString
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::RunString(FILE *scriptFile, int preparedInput)
{
    if (scriptFile == 0) {
        return 0;
    }

    hasPreparedInput = preparedInput;
    currentScriptFile = scriptFile;

    int readOk = 0;
    int runOk = 0;
    do {
        memset(g_zInterp_LineBuffer, 0, sizeof(g_zInterp_LineBuffer));
        readOk = ReadLineOrPreparedTokens(currentScriptFile, g_zInterp_LineBuffer);
        runOk = RunStream(g_zInterp_LineBuffer);
    } while ((readOk & runOk) != 0);

    hasPreparedInput = 0;
    return 1;
}

// Reimplements 0x4c1090: zInterp_Context::RunStream
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::RunStream(char *lineBuffer)
{
    if (lineBuffer == 0) {
        return 0;
    }

    const int wasPreparedInput = hasPreparedInput;
    parseResult = 0;
    lineHadError = 0;
    errorCount = 0;

    if (wasPreparedInput == 0) {
        TokenizeLine(lineBuffer);
    }

    if (tokenCount != 0) {
        char *const commandToken = CurrentCommandToken(this);
        if (HandleBuiltinCommand(commandToken) != 0) {
            const zInterp_Context_VTable *const table = RuntimeVTable(this);
            tokenReadIndex = 1;
            if (table->preDispatch(this, commandToken) != 0) {
                tokenReadIndex = 1;
                DispatchCoreCommand(commandToken);
                tokenReadIndex = 1;
                table->postDispatch(this, commandToken);
                if (errorCount == 3) {
                    Logf(this, "BadCommand (%s): %s", commandToken, lineBuffer);
                }
            }
        }

        if (lineHadError != 0) {
            RuntimeVTable(this)->deferredHook(this, commandToken);
        }
    }

    if (tempAlloc != 0) {
        free(tempAlloc);
    }
    tempAlloc = 0;
    return parseResult == 0 ? 1 : 0;
}

// Reimplements 0x4c13c0: zInterp_Context::TokenizeLine
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::TokenizeLine(const char *line)
{
    if (hasPreparedInput != 0) {
        return 1;
    }

    tokenCount = 0;
    tokenReadIndex = 1;

    const char *const comment = strchr(line, '#');
    if (comment == 0) {
        tempAlloc = _strdup(line);
    } else {
        const size_t prefixLength = static_cast<size_t>(comment - line);
        tempAlloc = static_cast<char *>(malloc(prefixLength + 1));
        memcpy(tempAlloc, line, prefixLength);
        tempAlloc[prefixLength] = '\0';
    }

    char *cursor = tempAlloc;
    while (isspace(static_cast<unsigned char>(*cursor)) != 0) {
        ++cursor;
    }

    char *separator = strpbrk(cursor, kTokenDelimiters);
    while (separator != 0) {
        tokenList[tokenCount] = cursor;
        ++tokenCount;

        cursor = separator + 1;
        const char separatorChar = *separator;
        *separator = '\0';
        if (separatorChar == '\n') {
            break;
        }

        while (isspace(static_cast<unsigned char>(*cursor)) != 0) {
            ++cursor;
        }

        separator = strpbrk(cursor, kTokenDelimiters);
    }

    if (strlen(cursor) != 0) {
        tokenList[tokenCount] = cursor;
        ++tokenCount;
    }

    return 1;
}

// Reimplements 0x4c1870: zInterp_Context::EchoTokens
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zInterp_Context::EchoTokens()
{
    for (unsigned int tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex) {
        printf("%s ", tokenList[tokenIndex]);
    }

    return printf("\n");
}

// Reimplements 0x4c1160: zInterp_Context::ReadLineOrPreparedTokens
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::ReadLineOrPreparedTokens(FILE *scriptFile, char *lineBuffer)
{
    if (hasPreparedInput == 0) {
        int ch = fgetc(scriptFile);
        if (ch != 0) {
            while (feof(scriptFile) == 0) {
                *lineBuffer = static_cast<char>(ch);
                ++lineBuffer;
                if (ch == '\n') {
                    break;
                }

                ch = fgetc(scriptFile);
                if (ch == 0) {
                    break;
                }
            }
        }

        return (feof(scriptFile) || ferror(scriptFile)) ? 0 : 1;
    }

    unsigned int tokenBlobSize = 0;
    tokenReadIndex = 1;
    fread(&tokenBlobSize, 4, 1, scriptFile);
    if (tokenBlobSize == 0) {
        tokenCount = 0;
        tempAlloc = 0;
        return 0;
    }

    fread(&tokenCount, 4, 1, scriptFile);
    tempAlloc = static_cast<char *>(malloc(tokenBlobSize));
    fread(tempAlloc, tokenBlobSize, 1, scriptFile);

    char *tokenText = tempAlloc;
    for (unsigned int tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex) {
        tokenList[tokenIndex] = tokenText;
        tokenText += strlen(tokenText) + 1;
    }

    return 1;
}

// Reimplements 0x4c1960: zInterp_Context::ClearFileFrameStack
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zInterp_Context::ClearFileFrameStack()
{
    if (fileFrameStack != 0) {
        free(fileFrameStack);
        fileFrameStack = 0;
    }
    fileFrameCount = 0;
}

// Reimplements 0x4c1940: zInterp_Context::PopFileFrame
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE zInterp_FileFrame *RECOIL_THISCALL zInterp_Context::PopFileFrame()
{
    int count = fileFrameCount;
    if (count == 0) {
        return 0;
    }

    --count;
    fileFrameCount = count;
    return &fileFrameStack[count];
}

// Reimplements 0x4c18c0: zInterp_Context::PushFileFrame
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::PushFileFrame(FILE *file, long filePos, int hasPreparedInput)
{
    zInterp_FileFrame *const frames = static_cast<zInterp_FileFrame *>(
        realloc(fileFrameStack, (fileFrameCount + 1) * sizeof(zInterp_FileFrame)));
    const int frameIndex = fileFrameCount;
    fileFrameStack = frames;

    frames[frameIndex].file = file;
    fileFrameStack[fileFrameCount].filePos = filePos;
    fileFrameStack[fileFrameCount].hasPreparedInput = hasPreparedInput;
    ++fileFrameCount;
    return 1;
}

// Reimplements 0x4c1c50: zInterp_Context::HandleBuiltinCommand
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::HandleBuiltinCommand(char *commandToken)
{
    if (strncmp(commandToken, "endif", 5) == 0) {
        if (conditionalDepth != 0) {
            --conditionalDepth;
            return 0;
        }
    } else if (conditionalDepth != 0) {
        return 0;
    }

    if (strncmp(commandToken, "ifdef", 5) == 0) {
        if (EvalConditionExpr() == 0) {
            ++conditionalDepth;
        }
        return 0;
    }

    if (strncmp(commandToken, "ifndef", 6) == 0) {
        if (EvalConditionExpr() != 0) {
            ++conditionalDepth;
        }
        return 0;
    }

    if (strncmp(commandToken, "mkdir", 5) == 0) {
        char *const path = NextToken();
        if (_mkdir(path) != 0 && errno != EEXIST) {
            ReportErrorf(this, "%s %s FAILED (errno == %d)", commandToken, path, errno);
        }
        return 0;
    }

    if (strcmp(commandToken, "Quit") == 0) {
        parseResult = 1;
        return 0;
    }

    if (strncmp(commandToken, "set", 3) == 0) {
        char *const name = NextToken();
        char *const value = NextToken();
        SetMacro(name, value);
        return 0;
    }

    if (strncmp(commandToken, "source", 6) == 0) {
        char *const sourcePath = NextToken();
        if (sourcePath != 0) {
            char *const filePath = _strdup(sourcePath);
            free(tempAlloc);
            tempAlloc = 0;
            RunScriptFile(filePath);
            free(filePath);
        }
        parseResult = 0;
        return 0;
    }

    if (strncmp(commandToken, "who", 3) == 0) {
        Logf(this, "%d Macros", macroCount);
        for (unsigned int i = 0; i < macroCount; ++i) {
            Logf(this, "%12s: %s", macroTable[i].name, macroTable[i].value);
        }
        Logf(this, "%d Variables", varCount);
        for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex) {
            DumpVarEntry(&varTable[varIndex]);
        }
        return 0;
    }

    if (strncmp(commandToken, "var", 3) == 0) {
        char *const name = NextToken();
        zInterp_VarEntry *const entry = FindVarEntry(name);
        if (entry == 0) {
            Logf(this, "Can't find variable ( %s )", name);
            return 0;
        }

        char *const op = NextToken();
        if (strncmp(op, "set", 3) != 0 && strncmp(op, &g_zInterp_AssignToken_Equal, 1) != 0) {
            IncErrorCount();
            return 0;
        }

        if (entry->type == 0) {
            *entry->valuePtr.intPtr = ParseIntToken();
            DumpVarEntry(entry);
            return 0;
        }
        if (entry->type == 1) {
            *entry->valuePtr.floatPtr = ParseFloatToken();
            DumpVarEntry(entry);
            return 0;
        }
        if (entry->type == 2) {
            Logf(this, "Can't modify string variables (yet!)");
            DumpVarEntry(entry);
            return 0;
        }

        DumpVarEntry(entry);
        return 0;
    }

    return 1;
}

// Reimplements 0x4c20a0: zInterp_Context::DispatchCoreCommand
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
zInterp_Context::DispatchCoreCommand(char *commandToken)
{
    if (CommandIs(this, "Echo") != 0 || CommandIs(this, "echo") != 0) {
        EchoTokens();
        return 1;
    }

    if (CommandIs(this, "FindNode") != 0) {
        char *const searchName = NextToken();
        currentNode = zClass::FindByTypeAndName(6, searchName);
        if (currentNode == 0) {
            ReportErrorf(this, "FindNode %s: FAILED", searchName);
        }
        return 1;
    }

    if (CommandIs(this, "FindSubNode") != 0) {
        char *const name = NextToken();
        currentNode =
            zClass_Class::FindSubNodeByName(static_cast<zClass_NodePartial *>(currentNode), name);
        if (currentNode == 0) {
            ReportErrorf(this, "FindSubNode %s: FAILED", name);
        }
        return 1;
    }

    if (CommandIs(this, "FreeNode") != 0) {
        char *const searchName = NextToken();
        zClass_NodePartial *const node = zClass::FindByTypeAndName(6, searchName);
        int result = 1;
        if (node != 0) {
            switch (node->classId) {
            case 1:
            case 3:
            case 5:
                result = zClass_Object3D::DeleteNode(node);
                break;
            case 2:
                result = zClass_World::DeleteNode(node);
                break;
            default:
                printf("Unrecognized node class = %d\n", node->classId);
                result = 1;
                break;
            }
        }
        if (result != 0) {
            printf("   Error freeing node %s\n", searchName);
        }
        return 1;
    }

    if (CommandIs(this, "GameZReadZBDFile") != 0) {
        char *const filename = NextToken();
        if (GameZ::ReadZBDFile(filename) != 0) {
            ReportErrorf(this, "%s %s FAILED", commandToken, filename);
        }
        return 1;
    }

    if (CommandIs(this, "GameZWriteZBDFile") != 0) {
        char *const filename = NextToken();
        zClass_Class::gwNodeUpdateAll();
        zClass::ProcessDeferredWork();
        GameZ::WriteZBDFile(filename);
        return 1;
    }

    if (CommandIs(this, "GetBFETolerance") != 0) {
        Logf(this, "BFE Tolerance: %.7f", zModel::GetBackfaceEliminationToleranceScalar());
        return 1;
    }

    if (CommandIs(this, "AddChild") != 0) {
        char *const searchName = NextToken();
        zClass_NodePartial *const child = zClass::FindByTypeAndName(6, searchName);
        zClass_NodePartial *const parent = static_cast<zClass_NodePartial *>(currentNode);
        if (parent == 0) {
            ReportErrorf(this, "%s %s FAILED because current node is NULL", commandToken,
                         searchName);
            return 1;
        }
        if (child == 0) {
            ReportErrorf(this, "%s %s FAILED because node [%s] wasn't found", commandToken,
                         searchName, searchName);
            return 1;
        }
        zClass_Class::AddChild(parent, child);
        return 1;
    }

    if (CommandHasPrefix(this, "AnimSetZBDFile") != 0) {
        zEffect_Anim::SetZbdFilename(NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "AnimSetDebugFrame") != 0) {
        zEffect::SetAnimDebugFrameTag();
        return 1;
    }

    if (CommandIs(this, "DeleteChild") != 0) {
        char *const name = NextToken();
        zClass_NodePartial *const parent = static_cast<zClass_NodePartial *>(currentNode);
        zClass_NodePartial *const child = zClass_Class::FindSubNodeByName(parent, name);
        if (parent != 0 && child != 0) {
            zClass_Class::RemoveChild(parent, child);
        } else {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x1c6, "interp: DeleteChild (%s, %s) --> NULL NODE",
                              parent != 0 ? parent->name : "NULL", name);
        }
        return 1;
    }

    if (CommandIs(this, "DeleteFile") != 0) {
        DeleteFileA(NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "DeleteTree") != 0) {
        char *const searchName = NextToken();
        zClass_NodePartial *const node = zClass::FindByTypeAndName(6, searchName);
        if (node != 0) {
            zClass_Util::DestroyNodeRecursive(node);
        } else {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x1de, "interp: DeleteTree (%s) --> NULL NODE", searchName);
        }
        return 1;
    }

    if (CommandIs(this, "CountCameraNodes") != 0) {
        printf("# of nodes in camera list = %d\n", zClass_TypeList::CountNodes(8));
        return 1;
    }

    if (CommandIs(this, "CountUsedNodes") != 0) {
        printf("# of nodes in used list = %d\n", zClass_TypeList::CountNodes(6));
        return 1;
    }

    if (CommandIsExact(this, "CameraGetTranslate") != 0) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        zClass_NodePartial *const camera = static_cast<zClass_NodePartial *>(currentNode);
        if (camera != 0) {
            zClass_Camera::gwCameraGetTarget(camera, &x, &y, &z);
        }
        Logf(this, "%s --> ( %.2f %.2f %.2f )", camera != 0 ? camera->name : "NULL", x, y, z);
        return 1;
    }

    if (CommandIs(this, "CameraSetActive") != 0) {
        zClass_Class::gwNodeSetActive(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "CameraSetDynamicLOD") != 0) {
        zClass_Camera::SetViewDistance(1, ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "CameraRotate") != 0) {
        const float x = ParseFloatToken();
        const float y = ParseFloatToken();
        const float z = ParseFloatToken();
        zClass_Camera::gwCameraSetPosition(static_cast<zClass_NodePartial *>(currentNode), x, y, z);
        return 1;
    }

    if (CommandIs(this, "CameraTranslate") != 0) {
        const float x = ParseFloatToken();
        const float y = ParseFloatToken();
        const float z = ParseFloatToken();
        zClass_Camera::gwCameraSetTarget(static_cast<zClass_NodePartial *>(currentNode), x, y, z);
        return 1;
    }

    if (CommandIs(this, "CameraSetFOV") != 0) {
        const float x = ParseFloatToken() * 0.01745329251994f;
        const float y = ParseFloatToken() * 0.01745329251994f;
        zClass_Camera::gwCameraSetFOV(static_cast<zClass_NodePartial *>(currentNode), x, y);
        return 1;
    }

    if (CommandHasPrefix(this, "CameraSetLODMultiplier") != 0) {
        zClass_Camera::gwCameraSetClipDistance(static_cast<zClass_NodePartial *>(currentNode),
                                               ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "CameraSetNearFarClip") != 0) {
        const float nearClip = ParseFloatToken();
        const float farClip = ParseFloatToken();
        zClass_Camera::gwCameraSetNearFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                              nearClip, farClip);
        return 1;
    }

    if (CommandIsExact(this, "CameraSetNearClip") != 0) {
        float nearClip = 0.0f;
        float farClip = 0.0f;
        zClass_Camera::gwCameraGetNearFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                              &nearClip, &farClip);
        nearClip = ParseFloatToken();
        zClass_Camera::gwCameraSetNearFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                              nearClip, farClip);
        return 1;
    }

    if (CommandIsExact(this, "CameraSetFarClip") != 0) {
        float nearClip = 0.0f;
        float farClip = 0.0f;
        zClass_Camera::gwCameraGetNearFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                              &nearClip, &farClip);
        farClip = ParseFloatToken();
        zClass_Camera::gwCameraSetNearFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                              nearClip, farClip);
        return 1;
    }

    if (CommandHasPrefix(this, "CameraSetObjectHSETest") != 0) {
        zClass_Camera::SetObjectHseTestEnabled(ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "CameraSetWindow") != 0) {
        zClass_NodePartial *const window = zClass::FindByTypeAndName(14, NextToken());
        zClass_Camera::gwCameraSetWindow(static_cast<zClass_NodePartial *>(currentNode), window);
        return 1;
    }

    if (CommandIs(this, "CameraSetWorld") != 0) {
        zClass_NodePartial *const world = zClass::FindByTypeAndName(13, NextToken());
        zClass_Camera::gwCameraSetWorld(static_cast<zClass_NodePartial *>(currentNode), world);
        return 1;
    }

    if (CommandIs(this, "CameraSetHorizonXZ") != 0) {
        zClass_NodePartial *const horizon = zClass::FindByTypeAndName(6, NextToken());
        zClass_Camera::gwCameraSetHorizonXZ(static_cast<zClass_NodePartial *>(currentNode),
                                            horizon);
        return 1;
    }

    if (CommandIs(this, "CameraSetHorizon") != 0) {
        zClass_NodePartial *const horizon = zClass::FindByTypeAndName(6, NextToken());
        zClass_Camera::gwCameraSetHorizon(static_cast<zClass_NodePartial *>(currentNode),
                                          horizon);
        return 1;
    }

    if (CommandIs(this, "DisplayOrigin") != 0) {
        const int x = ParseIntToken();
        const int y = ParseIntToken();
        zClass_Display::gwDisplaySetPosition(static_cast<zClass_NodePartial *>(currentNode), x, y);
        return 1;
    }

    if (CommandIs(this, "DisplayResolution") != 0) {
        const int width = ParseIntToken();
        const int height = ParseIntToken();
        zClass_Display::gwDisplaySetSize(static_cast<zClass_NodePartial *>(currentNode), width,
                                         height);
        return 1;
    }

    if (CommandIs(this, "DisplaySetClearColor") != 0) {
        const float r = ParseFloatToken();
        const float g = ParseFloatToken();
        const float b = ParseFloatToken();
        zClass_Display::gwDisplaySetBackgroundColor(
            static_cast<zClass_NodePartial *>(currentNode), r, g, b);
        return 1;
    }

    if (CommandHasPrefix(this, "CycleTextureSetLooping") != 0) {
        zModel_Instance::SetCycleTextureLoop(g_zInterp_CurrentCycleTextureDi, ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "CycleTextureSetMap") != 0) {
        zImage_TexDirEntryPartial *const texDirEntry = zImage::TexDir_FindOrAppendByPath(NextToken());
        zModel_Instance::AddCycleTexture(g_zInterp_CurrentCycleTextureDi, texDirEntry);
        return 1;
    }

    if (CommandHasPrefix(this, "CycleTextureSetOn") != 0) {
        const int textureCount = ParseIntToken();
        zClass_NodePartial *const node = static_cast<zClass_NodePartial *>(currentNode);
        if (node == 0) {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x198,
                              "No current node to enable cycle textures.  Take note of preceding "
                              "\"FindNode\" Error");
            return 1;
        }

        zClass_Class::gwNodeGetUserData(node, &g_zInterp_NodeUserDataScratch);
        g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
        if (g_zInterp_CurrentCycleTextureDi == 0) {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x1a2, "ERROR no GFX data for cycled texture (%s)", node->name);
            return 1;
        }

        if (zDi::SetCurrentVariantCycleTextureCount(g_zInterp_CurrentCycleTextureDi,
                                                    textureCount) == 0) {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x1a8,
                              "Node (%s) has no graphics data for cycled texture\n", node->name);
        }
        return 1;
    }

    if (CommandHasPrefix(this, "CycleTextureSetSpeed") != 0) {
        zDi::SetCurrentVariantCycleTextureSpeed(g_zInterp_CurrentCycleTextureDi,
                                                ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "ClearScreenBuffer") != 0) {
        zVideo::ExchangeClearScreenBufferEnabled(ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "LightNew") != 0) {
        currentNode = zClass_Light::gwLightNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "LightSetActive") != 0) {
        zClass_Class::gwNodeSetActive(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "LightSetAmbient") != 0) {
        zClass_Light::gwLightSetIntensity(static_cast<zClass_NodePartial *>(currentNode),
                                          ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "LightSetColor") != 0) {
        const float red = ParseFloatToken();
        const float green = ParseFloatToken();
        const float blue = ParseFloatToken();
        zClass_Light::gwLightSetSpecularColor(static_cast<zClass_NodePartial *>(currentNode),
                                              red, green, blue);
        return 1;
    }

    if (CommandIs(this, "LightSetDiffuse") != 0) {
        zClass_Light::gwLightSetFalloff(static_cast<zClass_NodePartial *>(currentNode),
                                        ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "LightSetDirectedSource") != 0) {
        zClass_Light::gwLightSetPointMode(static_cast<zClass_NodePartial *>(currentNode));
        return 1;
    }

    if (CommandIs(this, "LightSetDirectional") != 0) {
        zClass_Light::gwLightSetConeAngle(static_cast<zClass_NodePartial *>(currentNode),
                                          static_cast<unsigned int>(ParseBoolToken()));
        return 1;
    }

    if (CommandIs(this, "LightSetTranslate") != 0) {
        const float x = ParseFloatToken();
        const float y = ParseFloatToken();
        const float z = ParseFloatToken();
        zClass_Light::gwLightSetPosition(static_cast<zClass_NodePartial *>(currentNode), x, y, z);
        return 1;
    }

    if (CommandIs(this, "LightSetOrientation") != 0) {
        const float x = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        const float y = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        const float z = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        zClass_Light::gwLightSetRotation(static_cast<zClass_NodePartial *>(currentNode), x, y, z);
        return 1;
    }

    if (CommandIs(this, "LightSetRanges") != 0) {
        const float rangeA = ParseFloatToken();
        const float rangeB = ParseFloatToken();
        zClass_Light::gwLightSetRange(static_cast<zClass_NodePartial *>(currentNode), rangeA,
                                      rangeB);
        return 1;
    }

    if (CommandIs(this, "LightSetPointSource") != 0) {
        zClass_Light::gwLightSetDirectionalMode(static_cast<zClass_NodePartial *>(currentNode));
        return 1;
    }

    if (CommandIs(this, "LightSetSaturated") != 0) {
        if (currentNode == 0) {
            ReportErrorf(this, "%s Failed: no current node", commandToken);
            return 1;
        }

        zClass_Light::gwLightSetParam(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "LoadSoils") != 0) {
        zRndr_GlobalStringTable::LoadDynamicEntriesFromPath(NextToken());
        return 1;
    }

    if (CommandIs(this, "LensFlareTexture") != 0) {
        const int stageIndex = ParseIntToken();
        zImage_TexDirEntryPartial *const texDirEntry =
            zImage::TexDir_FindOrAppendByPath(NextToken());
        zRndr_LensFlare_SetVisibleSampleStage(stageIndex, texDirEntry);
        return 1;
    }

    if (CommandIs(this, "MatlFaceColor") != 0) {
        runtimeBlob->material.colorRgb.red = ParseFloatToken();
        runtimeBlob->material.colorRgb.green = ParseFloatToken();
        runtimeBlob->material.colorRgb.blue = ParseFloatToken();
        runtimeBlob->material.packedColor = zVid_PackColorRgbFloats(
            (zVideo_ColorRgbFloat *)(&runtimeBlob->material.colorRgb));
        return 1;
    }

    if (CommandIs(this, "MatlNew") != 0) {
        zModel_Material::ResetDefaults(&runtimeBlob->material);
        return 1;
    }

    if (CommandIs(this, "MatlTexture") != 0) {
        runtimeBlob->material.currentTextureDirectoryEntry =
            zImage::FindTexDirEntryByName(NextToken());
        if (runtimeBlob->material.currentTextureDirectoryEntry != 0) {
            runtimeBlob->material.flags |= 0x0100;
        } else {
            runtimeBlob->material.flags &= 0xfeff;
        }
        return 1;
    }

    if (CommandIs(this, "ModelNew") != 0) {
        currentNode = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        runtimeBlob->displayInstance = zModel_DiPool::AllocFromFreeList();
        zClass_Class::gwNodeSetDisplayInstance(static_cast<zClass_NodePartial *>(currentNode),
                                               runtimeBlob->displayInstance);
        if (strncmp(NextToken(), "Facade", 6) == 0) {
            zUtil::StoreInt32((int *)runtimeBlob->displayInstance, 1);
        } else {
            zUtil::StoreInt32((int *)runtimeBlob->displayInstance, 0);
        }
        return 1;
    }

    if (CommandIs(this, "ModelPolygonBegin") != 0) {
        runtimeBlob->pointCount = 0;
        runtimeBlob->uvCount = 0;
        runtimeBlob->normalsA = 0;
        return 1;
    }

    if (CommandIs(this, "ModelPolygonEnd") != 0) {
        runtimeBlob->polygonMaterial = zModel_Material::FindOrClone(&runtimeBlob->material);
        zTag4::Clear(&runtimeBlob->variantTag);
        zDi::AddPolygon(runtimeBlob->displayInstance, runtimeBlob->pointCount,
                        runtimeBlob->polygonPoints, runtimeBlob->uvPairs, runtimeBlob->normalsA,
                        runtimeBlob->normalsB, runtimeBlob->secondaryUvPairs,
                        runtimeBlob->polygonMaterial, runtimeBlob->drawFlags,
                        runtimeBlob->flagBit8, &runtimeBlob->variantTagWord);
        return 1;
    }

    if (CommandIs(this, "ModelPolygonUV") != 0) {
        zClipUV *const uv = &runtimeBlob->uvPairs[runtimeBlob->uvCount];
        uv->u = ParseFloatToken();
        uv->v = ParseFloatToken();
        ++runtimeBlob->uvCount;
        return 1;
    }

    if (CommandIs(this, "ModelPolygonVertex") != 0) {
        zVec3 *const point = &runtimeBlob->polygonPoints[runtimeBlob->pointCount];
        point->x = ParseFloatToken();
        point->y = ParseFloatToken();
        point->z = ParseFloatToken();
        ++runtimeBlob->pointCount;
        return 1;
    }

    if (CommandIs(this, "NewCamera") != 0) {
        currentNode = zClass_Camera::gwCameraNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewDisplay") != 0) {
        currentNode = zClass_Display::gwDisplayInit();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewLOD") != 0) {
        currentNode = zClass_Lod::gwLodNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewNode") != 0) {
        currentNode = zClass_Class::AllocNodeFromFreeList();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "LODAddChild") != 0) {
        zClass_NodePartial *const child = zClass::FindByTypeAndName(6, NextToken());
        zClass_Lod::gwLodAddChild(static_cast<zClass_NodePartial *>(currentNode), child);
        return 1;
    }

    if (CommandIs(this, "LODSetRange") != 0) {
        const float nearRange = ParseFloatToken();
        const float farRange = ParseFloatToken();
        zClass_LodDataPartial *const lodData =
            (zClass_LodDataPartial *)(static_cast<zClass_NodePartial *>(currentNode)->classData);
        lodData->nearRangeSq = nearRange * nearRange;
        lodData->farRangeSq = farRange * farRange;
        return 1;
    }

    if (CommandIs(this, "NewObject3D") != 0) {
        currentNode = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewSEQ") != 0) {
        currentNode = zClass_Sequence::gwSequenceNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewWindow") != 0) {
        currentNode = zClass_Window::gwWindowNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NewWorld") != 0) {
        currentNode = zClass_World::gwWorldNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NodeSetActive") != 0) {
        zClass_Class::gwNodeSetActive(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "NodeSetDescription") != 0) {
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandIs(this, "NodeSetCanModify") != 0) {
        zClass_Class::gwNodeSetFlag16(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "NodeSetLighting") != 0) {
        zClass_Node::AssignInt32ToDiRecursive(static_cast<zClass_NodePartial *>(currentNode),
                                              ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "NodeSetOverwrite") != 0) {
        zClass_Class::gwNodeSetVertexAlphaOverride(
            static_cast<zClass_NodePartial *>(currentNode), ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "Object3DAddChild") != 0) {
        char *const searchName = NextToken();
        zClass_NodePartial *const child = zClass::FindByTypeAndName(6, searchName);
        zClass_NodePartial *const parent = static_cast<zClass_NodePartial *>(currentNode);
        if (parent != 0 && child != 0) {
            zClass_Object3D::gwObject3DAddChild(parent, child);
        } else {
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                              0x26f, "interp: Object3DAddChild (%s, %s) --> NULL NODE",
                              parent != 0 ? parent->name : "NULL", searchName);
        }
        return 1;
    }

    if (CommandIsExact(this, "Object3DGetTranslate") != 0) {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        zClass_NodePartial *const node = static_cast<zClass_NodePartial *>(currentNode);
        if (node != 0) {
            zClass_Object3D::gwObject3DGetPosition(node, &x, &y, &z);
        }
        Logf(this, "%s --> ( %.2f %.2f %.2f )", node != 0 ? node->name : "NULL", x, y, z);
        return 1;
    }

    if (CommandIs(this, "Object3DRegisterTexturesToWorld") != 0) {
        const int registerTextures = ParseBoolToken();
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
        if (registerTextures != 0) {
            g_zInterp_CurrentCycleTextureDi->flags |= 0x04;
        } else {
            g_zInterp_CurrentCycleTextureDi->flags &= ~0x04;
        }
        return 1;
    }

    if (CommandIs(this, "Object3DTranslate") != 0) {
        const float x = ParseFloatToken();
        const float y = ParseFloatToken();
        const float z = ParseFloatToken();
        zClass_Object3D::gwObject3DSetPosition(static_cast<zClass_NodePartial *>(currentNode), x,
                                               y, z);
        return 1;
    }

    if (CommandIs(this, "Object3DRotate") != 0) {
        const float x = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        const float y = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        const float z = static_cast<float>(ParseFloatToken() * kDegreesToRadians);
        zClass_Object3D::gwObject3DSetRotation(static_cast<zClass_NodePartial *>(currentNode), x,
                                               y, z);
        return 1;
    }

    if (CommandIs(this, "Object3DScale") != 0) {
        const float x = ParseFloatToken();
        const float y = ParseFloatToken();
        const float z = ParseFloatToken();
        zClass_Object3D::gwObject3DSetScale(static_cast<zClass_NodePartial *>(currentNode), x, y,
                                            z);
        return 1;
    }

    if (CommandIs(this, "Object3DSetActive") != 0) {
        zClass_Class::gwNodeSetActive(static_cast<zClass_NodePartial *>(currentNode),
                                      ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "Object3DSetActionPriority") != 0) {
        zClass_Class::gwNodeSetPriority(static_cast<zClass_NodePartial *>(currentNode),
                                        ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "Object3DSetColor") != 0) {
        const int colorMode = ParseIntToken();
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        zDi::SetObject3DColorModeForMaterials((zDiPartial *)g_zInterp_NodeUserDataScratch,
                                              colorMode);
        return 1;
    }

    if (CommandIs(this, "Object3DSetFacade") != 0) {
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
        zUtil::StoreInt32((int *)g_zInterp_CurrentCycleTextureDi, 1);
        return 1;
    }

    if (CommandIs(this, "Object3DSetOpacityIsSet") != 0) {
        zClass_Object3D::gwObject3DSetLitFlag(static_cast<zClass_NodePartial *>(currentNode),
                                             ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "Object3DSetOpacity") != 0) {
        zClass_Object3D::gwObject3DSetAlphaScale(static_cast<zClass_NodePartial *>(currentNode),
                                                 ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "Object3DSetPoints") != 0) {
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
        zUtil::StoreInt32((int *)g_zInterp_CurrentCycleTextureDi, 2);
        return 1;
    }

    if (CommandIs(this, "Object3DSetPriority") != 0) {
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        zDi::SetEntryValueForAllEntries((zDiPartial *)g_zInterp_NodeUserDataScratch,
                                        static_cast<unsigned int>(ParseIntToken()));
        return 1;
    }

    if (CommandIs(this, "Object3DSetScrollAlways") != 0) {
        const int enabled = ParseBoolToken();
        const float textureWorldPerMeter = ParseFloatToken();
        const int textureWorldAxis = static_cast<int>(ParseFloatToken());
        if (enabled == 0) {
            DefaultDispatchHook(static_cast<zClass_NodePartial *>(currentNode));
            return 1;
        }
        if (RegisterScrollAlwaysNode(static_cast<zClass_NodePartial *>(currentNode),
                                     textureWorldPerMeter, textureWorldAxis, 1) == 0) {
            zError::ReportOld(
                0x200, "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp", 0x462,
                "Object3DSetScrollAlways on: FAILED  (node=0x%08x) (gfx=0x%08x)",
                currentNode, g_zInterp_CurrentCycleTextureDi);
        }
        return 1;
    }

    if (CommandIs(this, "Object3DSetScroll") != 0) {
        const int enabled = ParseBoolToken();
        const float textureWorldPerMeter = ParseFloatToken();
        const int textureWorldAxis = static_cast<int>(ParseFloatToken());
        zClass_NodePartial *const node = static_cast<zClass_NodePartial *>(currentNode);
        if (node != 0) {
            if (enabled == 0) {
                DefaultDispatchHook(node);
            } else {
                RegisterScrollAlwaysNode(node, textureWorldPerMeter, textureWorldAxis, 0);
            }
            return 1;
        }

        ReportErrorf(this, "%s %s %.1f %.1f Failed: current_node is NULL", commandToken,
                     enabled != 0 ? "ON" : "OFF", textureWorldPerMeter,
                     static_cast<float>(textureWorldAxis));
        return 1;
    }

    if (CommandIs(this, "Object3DSetShowBackFace") != 0) {
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        zDi::SetShowBackFaceForAllEntries((zDiPartial *)g_zInterp_NodeUserDataScratch,
                                          ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "Object3DSetTextureWorldBaseCoordinates") != 0) {
        const float baseU = ParseFloatToken();
        const float baseV = ParseFloatToken();
        zModel::SetTextureWorldBase(baseU, baseV);
        return 1;
    }

    if (CommandIs(this, "Object3DSetTextureWorldTexturesPerMeter") != 0) {
        const float worldPerMeterU = ParseFloatToken();
        const float worldPerMeterV = ParseFloatToken();
        zModel::SetTextureWorldPerMeter(worldPerMeterU, worldPerMeterV);
        return 1;
    }

    if (CommandIs(this, "Object3DSetMorphVertex") != 0) {
        const float blendY = ParseFloatToken();
        zClass_Class::gwNodeGetUserData(static_cast<zClass_NodePartial *>(currentNode),
                                        &g_zInterp_NodeUserDataScratch);
        zDiPartial *const di = (zDiPartial *)g_zInterp_NodeUserDataScratch;
        zDi::BuildBlendVertsFromConnectivity(di, 0, blendY, 0, 6);
        if (di != 0) {
            di->blendScale = 1.0f;
        }
        return 1;
    }

    if (CommandHasPrefix(this, "SEQAddChild") != 0) {
        char *const searchName = NextToken();
        const int insertIndex = ParseIntToken();
        const float delay = ParseFloatToken();
        zClass_NodePartial *const child = zClass::FindByTypeAndName(6, searchName);
        zClass_Sequence::gwSequenceAddChild(static_cast<zClass_NodePartial *>(currentNode), child,
                                            insertIndex, delay);
        return 1;
    }

    if (CommandHasPrefix(this, "SEQNew") != 0) {
        currentNode = zClass_Sequence::gwSequenceNew();
        zClass_Class::gwNodeSetName(static_cast<zClass_NodePartial *>(currentNode), NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SEQSetActive") != 0) {
        zClass_Sequence::SetActive(static_cast<zClass_NodePartial *>(currentNode),
                                   ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SEQSetLoop") != 0) {
        zClass_Sequence::SetLoop(static_cast<zClass_NodePartial *>(currentNode), ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SEQSetPause") != 0) {
        zClass_Sequence::SetPause(static_cast<zClass_NodePartial *>(currentNode), ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SEQSetRepeat") != 0) {
        zClass_Sequence::SetRepeat(static_cast<zClass_NodePartial *>(currentNode),
                                   ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetAltitudeSurface") != 0) {
        zClass_Class::gwNodeSetCellPickable(static_cast<zClass_NodePartial *>(currentNode),
                                            ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "SetGameZNodeArraySize") != 0) {
        zClass::SetNodeArraySize(ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "SetMaterialArraySize") != 0) {
        zModel_MatlBuffer::SetArraySize(ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "SetModel3DArraySize") != 0) {
        zModel::SetDisplayInstancePoolCapacity(ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "SetBFETolerance") != 0) {
        if (ValidateArgsAndNodeType(1, 0, 0) != 0) {
            zModel::SetBackfaceEliminationToleranceScalar(ParseFloatToken());
        }
        return 1;
    }

    if (CommandIs(this, "SetCoplanarTolerance") != 0) {
        zModel_Const::SetCoplanarTolerance(ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "SetColinearTolerance") != 0) {
        zModel_Const::SetColinearTolerance(ParseFloatToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetIntersectBBOX") != 0) {
        zClass_Class::gwNodeSetPickable(static_cast<zClass_NodePartial *>(currentNode),
                                        ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetIntersectSurface") != 0) {
        zClass_Class::gwNodeSetRaycastable(static_cast<zClass_NodePartial *>(currentNode),
                                           ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetLandmark") != 0) {
        zClass_Class::gwNodeSetBypassFarClip(static_cast<zClass_NodePartial *>(currentNode),
                                             ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "SetPaletteName") != 0) {
        zVideo::LoadPaletteFileAndApplyBrightness(NextToken());
        return 1;
    }

    if (CommandIs(this, "SetPaletteShading") != 0) {
        zModel::SetSoftwarePathActive(ParseBoolToken());
        return 1;
    }

    if (CommandIs(this, "SetInverseZTolerance") != 0) {
        zRndr::SetInverseZTolerance(ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "SetPerspectiveInverseZTolerance") != 0) {
        zRndr::SetPerspectiveAdaptiveCorrection(ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "SetPerspectiveTextureDeltaX") != 0) {
        zRndr::SetPerspectiveTextureDeltaX(ParseIntToken());
        return 1;
    }

    if (CommandIs(this, "SetPerspectiveTextureFarZ") != 0) {
        zRndr::SetPerspectiveTextureFarZ(ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "PerspectiveTexture") != 0) {
        zRndr::g_perspectiveTextureEnabled = ParseBoolToken();
        return 1;
    }

    if (CommandIs(this, "PrintNodeCount") != 0) {
        char *const prefixText = NextToken();
        zClass::FindNextByTypePrefix(prefixText, 6);
        int count = 0;
        while (zClass::FindNextByTypePrefix(0, 0) != 0) {
            ++count;
        }
        printf("Node count for %s = %d\n", prefixText, count);
        return 1;
    }

    if (CommandIsExact(this, "PrintTree") != 0) {
        if (currentNode == 0) {
            ReportErrorf(this, "No current node");
        }
        PrintNodeTree(static_cast<zClass_NodePartial *>(currentNode), 2);
        return 1;
    }

    if (CommandIs(this, "PrintUsedNodes") != 0) {
        zClass_TypeList::PrintBucket(6);
        return 1;
    }

    if (CommandIs(this, "RdrAddPath") != 0) {
        zUtil_ZRDR_AppendSearchPath(NextToken());
        return 1;
    }

    if (CommandIs(this, "RdrSetPath") != 0) {
        zUtil_ZRDR_SetSearchPath(NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetProximity") != 0) {
        zClass_Class::gwNodeSetHasHitCallback(static_cast<zClass_NodePartial *>(currentNode),
                                              ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetSmallPolygonRejectArea") != 0) {
        zModel::UpdateSmallPolyRejectThresholds(ParseFloatToken());
        return 1;
    }

    if (CommandIs(this, "SetPerspectiveAdaptiveCorrection") != 0) {
        const int minSpan = ParseIntToken();
        const int maxSpan = ParseIntToken();
        const float scale = ParseFloatToken();
        zRndr::SetPerspectiveAdaptiveSpanParams(minSpan, maxSpan, scale);
        return 1;
    }

    if (CommandHasPrefix(this, "SetTextureDirectory") != 0) {
        zImage_InitMissionResources(NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "SetVertexShading") != 0) {
        zModel::SetVertexShadingEnabled(ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "TextureAdd") != 0 || CommandIs(this, "AddEnhancerImage") != 0) {
        zImage::TexDir_FindOrAppendByPath(NextToken());
        return 1;
    }

    if (CommandHasPrefix(this, "Verbose") != 0) {
        g_zInterp_VerboseLevel = ParseBoolToken();
        return 1;
    }

    if (CommandIsExact(this, "VideoSetDither") != 0) {
        zVideo_dd3d::SetPendingDitherEnable(ParseBoolToken());
        return 1;
    }

    if (CommandIsExact(this, "VideoSetWireFrame") != 0) {
        zVideo_dd3d::SetPendingWireframeState(ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "WindowAddClearPolygonVertex") != 0) {
        if (ValidateArgsAndNodeType(3, 3, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            zVec3 point;
            point.x = ParseFloatToken();
            point.y = ParseFloatToken();
            point.z = ParseFloatToken();
            zClass_Window::gwWindowAddClearPolygonVertex(
                static_cast<zClass_NodePartial *>(currentNode), &point);
        }
        return 1;
    }

    if (CommandHasPrefix(this, "WindowBuffer") != 0) {
        if (ValidateArgsAndNodeType(1, 3, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            zClass_Window::gwWindowSetBuffer(static_cast<zClass_NodePartial *>(currentNode),
                                             ParseIntToken());
        }
        return 1;
    }

    if (CommandHasPrefix(this, "WindowCloseClearPolygon") != 0) {
        zClass_Window::gwWindowCloseClearPolygon(static_cast<zClass_NodePartial *>(currentNode));
        return 1;
    }

    if (CommandHasPrefix(this, "WindowOrigin") != 0) {
        const int width = ParseIntToken();
        const int height = ParseIntToken();
        zClass_Window::gwWindowSetSize(static_cast<zClass_NodePartial *>(currentNode), width,
                                       height);
        return 1;
    }

    if (CommandHasPrefix(this, "WindowResolution") != 0) {
        const int width = ParseIntToken();
        const int height = ParseIntToken();
        zClass_Window::gwWindowSetResolution(static_cast<zClass_NodePartial *>(currentNode), width,
                                             height);
        return 1;
    }

    if (CommandHasPrefix(this, "WindowSetClearPolygon") != 0) {
        zClass_Window::gwWindowSetClearPolygon(static_cast<zClass_NodePartial *>(currentNode),
                                               ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "WorldAddLight") != 0) {
        zClass_NodePartial *const light = zClass::FindByTypeAndName(9, NextToken());
        zClass_World::AddLight(static_cast<zClass_NodePartial *>(currentNode), light);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldExtents") != 0) {
        const float sizeX = ParseFloatToken();
        const float sizeZ = ParseFloatToken();
        zClass_World::gwWorldSetSize(static_cast<zClass_NodePartial *>(currentNode), sizeX,
                                     sizeZ);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldOrigin") != 0) {
        const float originX = ParseFloatToken();
        const float originZ = ParseFloatToken();
        zClass_World::gwWorldSetOrigin(static_cast<zClass_NodePartial *>(currentNode), originX,
                                       originZ);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldPartitionInclusionTolerance") != 0) {
        const float toleranceX = ParseFloatToken();
        const float toleranceZ = ParseFloatToken();
        zClass_World::gwWorldSetPartitionInclusionTolerance(
            static_cast<zClass_NodePartial *>(currentNode), toleranceX, toleranceZ);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldPartitionMaxDECFeatureCount") != 0) {
        zClass_World::gwWorldSetMaxDecFeatures(static_cast<zClass_NodePartial *>(currentNode),
                                               ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "WorldPartition") != 0) {
        const float cellSizeX = ParseFloatToken();
        const float cellSizeZ = ParseFloatToken();
        zClass_World::gwWorldSetVirtualAreaPartition(
            static_cast<zClass_NodePartial *>(currentNode), cellSizeX, cellSizeZ);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldSetFogAltitude") != 0) {
        const float minAlt = ParseFloatToken();
        const float maxAlt = ParseFloatToken();
        zClass_World::SetPendingFogAltitudeRange(static_cast<zClass_NodePartial *>(currentNode),
                                                 minAlt, maxAlt);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldSetFogColor") != 0) {
        const float red = ParseFloatToken();
        const float green = ParseFloatToken();
        const float blue = ParseFloatToken();
        zClass_World::SetPendingFogColorRgb01(static_cast<zClass_NodePartial *>(currentNode), red,
                                              green, blue);
        return 1;
    }

    if (CommandHasPrefix(this, "WorldSetFogDensity") != 0) {
        zClass_World::SetPendingFogDensity(static_cast<zClass_NodePartial *>(currentNode),
                                           ParseFloatToken());
        return 1;
    }

    if (CommandIsExact(this, "WorldSetFogRange") != 0) {
        if (ValidateArgsAndNodeType(2, 2, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            const float nearRange = ParseFloatToken();
            const float farRange = ParseFloatToken();
            zClass_World::SetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             nearRange, farRange);
        }
        return 1;
    }

    if (CommandIsExact(this, "WorldSetFogRangeNear") != 0) {
        if (ValidateArgsAndNodeType(1, 2, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            float nearRange = 0.0f;
            float farRange = 0.0f;
            zClass_World::GetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             &nearRange, &farRange);
            nearRange = ParseFloatToken();
            zClass_World::SetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             nearRange, farRange);
        }
        return 1;
    }

    if (CommandIsExact(this, "WorldSetFogRangeFar") != 0) {
        if (ValidateArgsAndNodeType(1, 2, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            float nearRange = 0.0f;
            float farRange = 0.0f;
            zClass_World::GetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             &nearRange, &farRange);
            farRange = ParseFloatToken();
            zClass_World::SetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             nearRange, farRange);
        }
        return 1;
    }

    if (CommandIsExact(this, "WorldGetFogRange") != 0) {
        if (ValidateArgsAndNodeType(0, 2, static_cast<zClass_NodePartial *>(currentNode)) != 0) {
            float nearRange = 0.0f;
            float farRange = 0.0f;
            zClass_World::GetPendingFogRange(static_cast<zClass_NodePartial *>(currentNode),
                                             &nearRange, &farRange);
            Logf(this, "Fog Range: [%s] [ %.2f, %.2f ]",
                 static_cast<zClass_NodePartial *>(currentNode)->name, nearRange, farRange);
        }
        return 1;
    }

    if (CommandHasPrefix(this, "WorldSetFogState") != 0) {
        if (ValidateArgsAndNodeType(1, 2, static_cast<zClass_NodePartial *>(currentNode)) == 0) {
            return 1;
        }

        char *const state = NextToken();
        if (strncmp(state, "linear", 6) == 0) {
            zClass_World::SetPendingFogState(static_cast<zClass_NodePartial *>(currentNode), 1);
        } else if (strncmp(state, "exponential", 11) == 0) {
            zClass_World::SetPendingFogState(static_cast<zClass_NodePartial *>(currentNode), 2);
        } else if (strncmp(state, "off", 3) == 0) {
            zClass_World::SetPendingFogState(static_cast<zClass_NodePartial *>(currentNode), 0);
        } else {
            printf("Did not understand: %s\n", state);
        }
        return 1;
    }

    if (CommandHasPrefix(this, "WorldSetVirtualPartition") != 0) {
        zClass_World::SetVirtualPartition(static_cast<zClass_NodePartial *>(currentNode),
                                          ParseBoolToken());
        return 1;
    }

    if (CommandHasPrefix(this, "WriteTextureSetType") != 0) {
        OptCatalog::SetDamageMaskSlotIndex(ParseIntToken());
        return 1;
    }

    if (CommandHasPrefix(this, "WriteTextureSetMap") != 0) {
        OptCatalog::RegisterDamageMaskSlotPtr(zImage::TexDir_FindOrAppendByPath(NextToken()));
        return 1;
    }

    IncErrorCount();
    return 1;
}

// Reimplements 0x4c2030: zInterp_Context::PrintNodeTree
// (D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
zInterp_Context::PrintNodeTree(zClass_NodePartial *node, int indent)
{
    if (node == 0) {
        return;
    }

    Logf(this, "%*s%s", indent, " ", node->name);

    const int childCount = node->listCountB;
    if (childCount <= 0) {
        return;
    }

    const int childIndent = indent + 2;
    for (int childIndex = 0; childIndex < childCount; ++childIndex) {
        PrintNodeTree(node->listB[childIndex], childIndent);
    }
}
