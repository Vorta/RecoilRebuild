#include "GameZRecoil/zInterp/zInterp.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zModel/zModel.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

namespace
{
const int kPreparedScriptMagic = 0x08971119;
const int kPreparedScriptVersion = 7;
const char kTokenDelimiters[] = ", \t\n";
char g_zInterp_MacroExpansionScratch[2048];
} // namespace

const int g_zInterp_Context_VTableMarker = 0;

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

    runtimeBlob = malloc(0x1d8);
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
    unknown_98 = 0;
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
