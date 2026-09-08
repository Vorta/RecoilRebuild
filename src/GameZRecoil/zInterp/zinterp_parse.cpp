#include "GameZRecoil/zInterp/zinterp.h"

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "Battlesport/wol_download.h"

#include <ctype.h>
#include <direct.h>
#include <errno.h>
#include <new>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

namespace {
const int kPreparedScriptMagic = 0x08971119;
const int kPreparedScriptVersion = 7;
const double kDegreesToRadians = 0.01745329251994;
const char kGlobalContextSearchPath[] = ".;zbd";
const char kCommandNameWeaponSetMaxTetherAltitude[] = "WeaponSetMaxTetherAltitude";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-k-zinterp-tokendelimiters
 * @recoil-artifact defines .data recoil:data:0x4e4918: k_zInterp_TokenDelimiters.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: char[0x5] ", \t\n"; TokenizeLine passes it to strpbrk
 * while splitting command tokens.
 *
 * Purpose: delimiter set used by zInterp_Context::TokenizeLine.
 */
const char k_zInterp_TokenDelimiters[] = ", \t\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-printtokenwithspacefmt
 * @recoil-artifact defines .data recoil:data:0x4e4920: g_zInterp_PrintTokenWithSpaceFmt.
 * Data owner: zInterp parser/runtime initialized format-string run.
 * BN evidence: writable char[0x4] "%s "; adjacent to the DumpVarEntry
 * format strings at 0x4e4924, 0x4e4934, and 0x4e4944.
 *
 * Purpose: printf format used by zInterp_Context::EchoTokens.
 */
char g_zInterp_PrintTokenWithSpaceFmt[] = "%s ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-k-zinterp-formatvarint
 * @recoil-artifact defines .data recoil:data:0x4e4924: k_zInterp_FormatVarInt.
 * Data owner: zInterp parser/runtime initialized format-string run.
 * BN evidence: writable char[0xe] "%s = (int) %d"; first DumpVarEntry
 * format string in the contiguous run that starts at 0x4e4920.
 *
 * Purpose: Logf format for integer script variable entries.
 */
char k_zInterp_FormatVarInt[] = "%s = (int) %d";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-k-zinterp-formatvarfloat
 * @recoil-artifact defines .data recoil:data:0x4e4934: k_zInterp_FormatVarFloat.
 * Data owner: zInterp parser/runtime initialized format-string run.
 * BN evidence: writable char[0x10] "%s = (float) %f"; second DumpVarEntry
 * format string in the contiguous run that starts at 0x4e4920.
 *
 * Purpose: Logf format for floating-point script variable entries.
 */
char k_zInterp_FormatVarFloat[] = "%s = (float) %f";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-k-zinterp-formatvarstring
 * @recoil-artifact defines .data recoil:data:0x4e4944: k_zInterp_FormatVarString.
 * Data owner: zInterp parser/runtime initialized format-string run.
 * BN evidence: writable char[0x11] "%s = (char*)\"%s\""; final DumpVarEntry
 * format string in the contiguous run that starts at 0x4e4920.
 *
 * Purpose: Logf format for character-pointer script variable entries.
 */
char k_zInterp_FormatVarString[] = "%s = (char*)\"%s\"";
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-linebuffer
 * @recoil-artifact defines .data recoil:data:0x56bf78: g_zInterp_LineBuffer.
 * Data owner: zInterp_Context initialized globals.
 *
 * Purpose: shared line buffer used while reading and running script input.
 */
char g_zInterp_LineBuffer[1024] = {0};

namespace {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-macroexpansionscratch
 * @recoil-artifact defines .data recoil:data:0x56c378: g_zInterp_MacroExpansionScratch.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: 0x56c378 is char[0x400] BSS; larger storage would overlap
 * distinct globals at 0x56c780, 0x56c784, and 0x56c788.
 *
 * Purpose: shared scratch buffer used while expanding macro references.
 */
char g_zInterp_MacroExpansionScratch[1024];
} // namespace

extern char g_zInterp_PreparedIndexFileNameStr[];

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-printnodetree-offstring
 * @recoil-artifact defines .data recoil:data:0x4de4c0: g_zInterp_PrintNodeTree_OffString.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: writable char[0x4] "OFF"; DispatchCoreCommand references it
 * when reporting Object3DSetScroll with a missing current node.
 *
 * Purpose: shared OFF text for Object3D scroll command diagnostics.
 */
char g_zInterp_PrintNodeTree_OffString[] = "OFF";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-enablepreparedscripts
 * @recoil-artifact defines .data recoil:data:0x4e48f0: g_zInterp_EnablePreparedScripts.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: 0x4e48f0 contains 01 00 00 00.
 *
 * Purpose: enables loading prepared script streams when the index is present.
 */
int g_zInterp_EnablePreparedScripts = 1;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-verboselevel
 * @recoil-artifact defines .data recoil:data:0x575ddc: g_zInterp_VerboseLevel.
 * Data owner: zInterp_Context initialized globals.
 *
 * Purpose: controls script parser logging verbosity.
 */
int g_zInterp_VerboseLevel = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-assigntoken-equal
 * @recoil-artifact defines .data recoil:data:0x4e4988: g_zInterp_AssignToken_Equal.
 * Data owner: zInterp_Context initialized globals.
 *
 * Purpose: single-character assignment token used by variable binding.
 */
char g_zInterp_AssignToken_Equal = '=';

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-k-zinterp-printnodetreeformat
 * @recoil-artifact defines .data recoil:data:0x4e4a28: k_zInterp_PrintNodeTreeFormat.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: writable char[0x6] "%*s%s"; PrintNodeTree passes it to Logf
 * for indentation and node-name output.
 *
 * Purpose: printf format used by zInterp_Context::PrintNodeTree.
 */
char k_zInterp_PrintNodeTreeFormat[] = "%*s%s";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-scrollalwaysnodename
 * @recoil-artifact defines .data recoil:data:0x4e5af8: g_zInterp_ScrollAlwaysNodeName.
 * Data owner: zInterp parser/runtime initialized object command literals.
 * BN evidence: writable char[0xd] "ScrollAlways"; RegisterScrollAlwaysNode
 * passes this storage to gwNodeSetName for the driver callback node.
 *
 * Purpose: node name assigned to the texture-scroll driver object.
 */
char g_zInterp_ScrollAlwaysNodeName[] = "ScrollAlways";

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-object3dcommandintscratch
 * @recoil-artifact defines .data recoil:data:0x56c780: g_zInterp_Object3DCommandDi.
 * Data owner: zInterp_Context initialized globals.
 * Retail evidence: the color, priority, and show-backface command handlers
 * store node user data here as a display-instance pointer. The latter two
 * reload it after parsing their integer argument. The descriptive name is
 * not a recovered original spelling.
 *
 * Purpose: preserve the display instance for Object3D material commands.
 */
zDiPartial *g_zInterp_Object3DCommandDi = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-currentcycletexturedi
 * @recoil-artifact defines .data recoil:data:0x56c784: g_zInterp_CurrentCycleTextureDi.
 * Data owner: zInterp_Context initialized globals.
 *
 * Purpose: holds the current display-instance cursor for texture commands.
 */
zDiPartial *g_zInterp_CurrentCycleTextureDi = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-nodeuserdatascratch
 * @recoil-artifact defines .data recoil:data:0x56c788: g_zInterp_NodeUserDataScratch.
 * Data owner: zInterp_Context initialized globals.
 *
 * Purpose: temporary user-data transfer slot for zClass node callbacks.
 */
unsigned int g_zInterp_NodeUserDataScratch = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-globalcontext
 * @recoil-artifact defines .data recoil:data:0x4edb78: g_zInterp_GlobalContext.
 * Data owner: zInterp_GlobalContext initialized instance. The ordinary
 * file-scope object lets VC5 emit its natural CRT construction and teardown
 * roots, which retain the derived constructor and virtual dispatch table.
 *
 * Purpose: process-wide script interpreter context constructed at startup.
 */
zInterp_GlobalContext g_zInterp_GlobalContext;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-preparedindexfilename
 * @recoil-artifact defines .data recoil:data:0x4e48f4: g_zInterp_PreparedIndexFileName.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: pointer storage follows g_zInterp_EnablePreparedScripts and
 * points at the mutable prepared-index filename storage immediately after it.
 *
 * Purpose: default prepared script index path passed to the global context.
 */
char *g_zInterp_PreparedIndexFileName = g_zInterp_PreparedIndexFileNameStr;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-g-zinterp-preparedindexfilenamestr
 * @recoil-artifact defines .data recoil:data:0x4e48f8: g_zInterp_PreparedIndexFileNameStr.
 * Data owner: zInterp_Context initialized globals.
 * BN evidence: mutable "interp.zbd" storage follows the prepared-index pointer.
 *
 * Purpose: backing storage for the default prepared script index path.
 */
char g_zInterp_PreparedIndexFileNameStr[] = "interp.zbd";

/**
 * Data owner: zInterp parser/runtime initialized file-mode literal.
 * BN evidence: writable char[0x2] "r"; RunScriptFile passes this exact
 * storage to fopen when prepared-script input is unavailable.
 *
 * Purpose: read-mode string used when opening plain script files.
 */
extern "C" char g_zEffectAnim_FileModeRead[] = "r";

/**
 * Candidate initialized-data preservation, not an accepted source owner.
 * Retail bytes can be represented by these floats, but the complete audit
 * found no base/interior or incoming pointer-table use. The former BN
 * float-array definition was withdrawn; original type and extent are unknown.
 *
 * Purpose: retain the unresolved initialized byte pattern without accepting
 * a float-defaults role or the current declaration's source identity.
 */
extern "C" float g_zInterp_UnresolvedFloatDefaults[63] = {
    0.0f, 2.2f, 0.2f,
    2.0f, -2.2f, 0.2f,
    -2.0f, -2.2f, -0.2f,
    2.0f, 2.2f, -0.2f,
    -2.0f, 0.0f, 5.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 5.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 5.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    -5.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f
};

namespace {
/**
 * Original inline helper evidence: no standalone retail function observed;
 * recovered from repeated zInterp_Context::DispatchCoreCommand comparisons.
 *
 * Purpose: compare the current command token against a literal prefix length.
 */
#define CommandIs(ctx, text) \
    (strncmp((ctx)->tokenCount > 0 ? (ctx)->tokenList[0] : 0, (text), sizeof(text) - 1) == 0)

/**
 * Original inline helper evidence: no standalone retail function observed;
 * recovered from exact zInterp_Context::DispatchCoreCommand comparisons.
 *
 * Purpose: compare the current command token against a full literal string.
 */
#define CommandIsExact(ctx, text) \
    (strcmp((ctx)->tokenCount > 0 ? (ctx)->tokenList[0] : 0, (text)) == 0)

/**
 * Original inline helper evidence: no standalone retail function observed;
 * recovered from prefix zInterp_Context::DispatchCoreCommand comparisons.
 *
 * Purpose: compare the current command token against a named prefix.
 */
#define CommandHasPrefix(ctx, text) \
    (strncmp((ctx)->tokenCount > 0 ? (ctx)->tokenList[0] : 0, (text), sizeof(text) - 1) == 0)
} // namespace

/**
 * Original helper evidence: zInterp_Context default virtual dispatch hook.
 *
 * Purpose: report an unhandled command token through the parser error path.
 */
int zInterp_Context::DispatchHook(
    char *commandToken
) {
    return ReportParseError(commandToken);
}

/**
 * Original helper evidence: zInterp_Context default virtual post-dispatch hook.
 *
 * Purpose: report an unhandled command token after core dispatch.
 */
int zInterp_Context::PostDispatchHook(
    char *commandToken
) {
    return ReportParseError(commandToken);
}

/**
 * Original helper evidence: zInterp_Context deferred virtual hook slot.
 * BN evidence: zInterp vtable slot 2 points at shared local no-op 0x414b50;
 * the same code is also referenced by non-zInterp data, so this declaration
 * documents the zInterp slot contract without claiming exclusive ownership.
 *
 * Purpose: provide a default deferred-command hook that accepts the command.
 */
int zInterp_Context::DeferredDispatchHook(
    char *
) {
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-constructor
 * @recoil-artifact defines .text recoil:function:0x4c0d20: zInterp_Context::zInterp_Context.
 *
 * Purpose: initialize one parser context, including prepared-script index
 * state, macro/variable tables, runtime scratch storage, and scroll callbacks.
 */
zInterp_Context::zInterp_Context(
    const char *preparedIndexPath,
    const char *searchPathText
) {
    includeDepth = 0;

    runtimeBlob = (zInterp_RuntimeBlob *)(malloc(sizeof(zInterp_RuntimeBlob)));
    memset(
        runtimeBlob,
        0,
        sizeof(*runtimeBlob)
    );

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
    memset(&preparedIndexHeader, 0, sizeof(preparedIndexHeader));

    preparedEntryCount = (int *)(malloc(sizeof(int)));
    *preparedEntryCount = 0;

    preparedEntryTable = 0;
    macroTable = 0;
    macroCount = 0;
    varTable = 0;
    varCount = 0;
    ptrArrayHead = 0;
    ptrArrayCount = 0;

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-destructor
 * @recoil-artifact defines .text recoil:function:0x4c0e50: zInterp_Context::~zInterp_Context.
 *
 * Purpose: tear down a context after Destroy has released active runtime state.
 */
zInterp_Context::~zInterp_Context() {
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

}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-destroy
 * @recoil-artifact defines .text recoil:function:0x4c0f70: zInterp_Context::Destroy.
 *
 * Purpose: clear per-run parser tables, search paths, scroll callbacks, and
 * pointer-array state while leaving constructor-owned storage intact.
 */
void zInterp_Context::Destroy() {
    ClearMacroTable();
    ClearVarTable();
    ClearFileFrameStack();

    if (archiveSearchList != 0) {
        zUtil_ZRDR_FreeSearchPathList(archiveSearchList);
        archiveSearchList = 0;
    }

    scrollAlwaysDriverNode = 0;

    scrollAlwaysList.clear();

    if (ptrArrayHead != 0) {
        free(ptrArrayHead);
    }
    ptrArrayHead = 0;
    ptrArrayCount = 0;
    includeDepth = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-runstring
 * @recoil-artifact defines .text recoil:function:0x4c1020: zInterp_Context::RunString.
 *
 * Purpose: read script input lines or prepared token blobs and run each one.
 */
int zInterp_Context::RunString(
    FILE *scriptFile,
    int preparedInput
) {
    if (scriptFile == 0) {
        return 0;
    }

    hasPreparedInput = preparedInput;
    currentScriptFile = scriptFile;

    int readOk = 0;
    do {
        memset(
            g_zInterp_LineBuffer,
            0,
            sizeof(g_zInterp_LineBuffer)
        );
        readOk = ReadLineOrPreparedTokens(
            currentScriptFile,
            g_zInterp_LineBuffer
        );
        readOk &= RunStream(g_zInterp_LineBuffer);
    } while (readOk != 0);

    hasPreparedInput = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-runstream
 * @recoil-artifact defines .text recoil:function:0x4c1090: zInterp_Context::RunStream.
 *
 * Purpose: tokenize one command line, dispatch builtins/core hooks, and clear
 * temporary token storage.
 */
int zInterp_Context::RunStream(
    char *lineBuffer
) {
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
        char *const commandToken = tokenCount > 0 ? tokenList[0] : 0;
        if (HandleBuiltinCommand(commandToken) != 0) {
            tokenReadIndex = 1;
            if (DispatchHook(commandToken) != 0) {
                tokenReadIndex = 1;
                DispatchCoreCommand(commandToken);
                tokenReadIndex = 1;
                PostDispatchHook(commandToken);
                if (errorCount == 3) {
                    Logf(
                        this,
                        "BadCommand (%s): %s",
                        commandToken,
                        lineBuffer
                    );
                }
            }
        }

        if (lineHadError != 0) {
            DeferredDispatchHook(commandToken);
        }
    }

    if (tempAlloc != 0) {
        free(tempAlloc);
    }
    tempAlloc = 0;
    return parseResult == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-readlineorpreparedtokens
 * @recoil-artifact defines .text recoil:function:0x4c1160: zInterp_Context::ReadLineOrPreparedTokens.
 *
 * Purpose: read either a text script line or a prepared token block.
 */
int zInterp_Context::ReadLineOrPreparedTokens(
    FILE *scriptFile,
    char *lineBuffer
) {
    if (hasPreparedInput == 0) {
        int ch;
        while ((ch = fgetc(scriptFile)) != 0 && feof(scriptFile) == 0) {
            *lineBuffer = (char)(ch);
            ++lineBuffer;
            if (ch == '\n') {
                break;
            }
        }

        return (feof(scriptFile) || ferror(scriptFile)) ? 0 : 1;
    }

    unsigned int tokenBlobSize;
    tokenReadIndex = 1;
    fread(
        &tokenBlobSize,
        4,
        1,
        scriptFile
    );
    if (tokenBlobSize == 0) {
        tokenCount = 0;
        tempAlloc = 0;
        return 0;
    }

    fread(
        &tokenCount,
        4,
        1,
        scriptFile
    );
    tempAlloc = (char *)(malloc(tokenBlobSize));
    fread(
        tempAlloc,
        tokenBlobSize,
        1,
        scriptFile
    );

    char *tokenText = tempAlloc;
    for (unsigned int tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex) {
        tokenList[tokenIndex] = tokenText;
        tokenText += strlen(tokenText) + 1;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-expandmacrorefs
 * @recoil-artifact defines .text recoil:function:0x4c1250: zInterp_Context::ExpandMacroRefs.
 *
 * Purpose: substitute percent-delimited macro references into a shared scratch
 * buffer before token parsing consumes the argument.
 */
char * zInterp_Context::ExpandMacroRefs(
    char *lineBuf
) {
    if (lineBuf == 0) {
        return 0;
    }

    char *open = strchr(
        lineBuf,
        '%'
    );
    char *close = open != 0 ? strchr(
        open + 1,
        '%'
    ) : 0;
    if (open == 0) {
        return lineBuf;
    }
    if (close == 0) {
        return lineBuf;
    }

    g_zInterp_MacroExpansionScratch[0] = '\0';
    char *segmentStart = lineBuf;
    char macroName[64];

    do {
        const int literalLength = (int)(open - segmentStart);
        if (literalLength != 0) {
            strncat(
                g_zInterp_MacroExpansionScratch,
                segmentStart,
                literalLength
            );
        }

        const int macroSpanLength = (int)(close - open);
        strncpy(
            macroName,
            open + 1,
            macroSpanLength
        );
        macroName[macroSpanLength - 1] = '\0';

        char *const value = FindMacroValue(
            macroName,
            0
        );
        if (value != 0) {
            strcat(
                g_zInterp_MacroExpansionScratch,
                value
            );
        }

        segmentStart = close + 1;
        open = strchr(
            segmentStart,
            '%'
        );
        close = open != 0 ? strchr(
            open + 1,
            '%'
        ) : 0;
    } while (open != 0 && close != 0);

    if (segmentStart != 0 && *segmentStart != '\0') {
        strcat(g_zInterp_MacroExpansionScratch, segmentStart);
    }

    const int trailingMacroSpan = (int)(close - open);
    if (trailingMacroSpan > 0) {
        strncpy(macroName, open + 1, trailingMacroSpan);
    }

    return g_zInterp_MacroExpansionScratch;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-tokenizeline
 * @recoil-artifact defines .text recoil:function:0x4c13c0: zInterp_Context::TokenizeLine.
 * BN evidence: retail imports iswspace and sign-extends token bytes before
 * each whitespace classification call.
 *
 * Purpose: copy a text line, strip comments, and split command tokens.
 */
int zInterp_Context::TokenizeLine(
    const char *line
) {
    if (hasPreparedInput != 0) {
        return 1;
    }

    tokenCount = 0;
    tokenReadIndex = 1;

    const char *const comment = strchr(
        line,
        '#'
    );
    if (comment == 0) {
        tempAlloc = _strdup(line);
    } else {
        const size_t prefixSize = strlen(line) - strlen(comment) + 1;
        tempAlloc = (char *)(malloc(prefixSize));
        memcpy(
            tempAlloc,
            line,
            prefixSize
        );
        tempAlloc[prefixSize - 1] = '\0';
    }

    char *cursor = tempAlloc;
    while (iswspace(*cursor) != 0) {
        ++cursor;
    }

    char *separator = strpbrk(
        cursor,
        k_zInterp_TokenDelimiters
    );
    while (separator != 0) {
        tokenList[tokenCount++] = cursor;

        cursor = separator + 1;
        if (*separator == '\n') {
            *separator = '\0';
            break;
        }
        *separator = '\0';

        while (iswspace(*cursor) != 0) {
            ++cursor;
        }

        separator = strpbrk(
            cursor,
            k_zInterp_TokenDelimiters
        );
    }

    if (strlen(cursor) != 0) {
        tokenList[tokenCount] = cursor;
        ++tokenCount;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-runscriptfile
 * @recoil-artifact defines .text recoil:function:0x4c1500: zInterp_Context::RunScriptFile.
 *
 * Purpose: push nested script state, choose prepared or text input, and run it.
 */
int zInterp_Context::RunScriptFile(
    const char *filePath
) {
    FILE *scriptFile = 0;
    int hasPrepared = 0;

    if (currentScriptFile != 0) {
        const long filePos = ftell(currentScriptFile);
        PushFileFrame(
            currentScriptFile,
            filePos,
            hasPreparedInput
        );
    }

    if (g_zInterp_EnablePreparedScripts != 0 &&
        LoadPreparedScriptIndex(preparedIndexFileName) != 0) {
        scriptFile = OpenPreparedScriptStream(filePath);
        if (scriptFile != 0) {
            hasPrepared = 1;
        }
    }

    if (scriptFile == 0) {
        scriptFile = fopen(
            filePath,
            g_zEffectAnim_FileModeRead
        );
        ++includeDepth;
    }

    int result = 0;
    if (scriptFile != 0) {
        result = RunString(
            scriptFile,
            hasPrepared
        );
        if (hasPrepared == 0) {
            fclose(scriptFile);
        }
    }

    zInterp_FileFrame *const frame = PopFileFrame();
    if (frame != 0) {
        currentScriptFile = frame->file;
        fseek(
            currentScriptFile,
            frame->filePos,
            0
        );
        hasPreparedInput = frame->hasPreparedInput;
    } else {
        currentScriptFile = 0;
    }
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-findmacrovalue
 * @recoil-artifact defines .text recoil:function:0x4c15f0: zInterp_Context::FindMacroValue.
 *
 * Purpose: locate a macro entry by name and optionally return its table slot.
 */
char * zInterp_Context::FindMacroValue(
    const char *name,
    zInterp_MacroEntry **outEntry
) {
    zInterp_MacroEntry *entry = macroTable;
    for (unsigned int macroIndex = 0; macroIndex < macroCount; ++macroIndex, ++entry) {
        if (strcmp(
            name,
            entry->name
        ) == 0) {
            if (outEntry != 0) {
                *outEntry = entry;
            }
            return entry->value;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-clearmacrotable
 * @recoil-artifact defines .text recoil:function:0x4c1670: zInterp_Context::ClearMacroTable.
 *
 * Purpose: free all macro names, values, and table storage for the context.
 */
void zInterp_Context::ClearMacroTable() {
    zInterp_MacroEntry *entry = macroTable;
    for (unsigned int macroIndex = 0; macroIndex < macroCount; ++macroIndex, ++entry) {
        free(entry->name);
        free(entry->value);
    }

    if (macroTable != 0) {
        free(macroTable);
    }
    macroTable = 0;
    macroCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-clearvartable
 * @recoil-artifact defines .text recoil:function:0x4c16c0: zInterp_Context::ClearVarTable.
 *
 * Purpose: free variable table names and release the context's table storage.
 */
void zInterp_Context::ClearVarTable() {
    zInterp_VarEntry *entry = varTable;
    for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex, ++entry) {
        free(entry->name);
    }

    if (varTable != 0) {
        free(varTable);
    }
    varTable = 0;
    varCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-ismacrotrue
 * @recoil-artifact defines .text recoil:function:0x4c1710: zInterp_Context::IsMacroTrue.
 *
 * Purpose: test whether a named macro currently holds the TRUE literal.
 */
int zInterp_Context::IsMacroTrue(
    const char *name
) {
    const char *const value = FindMacroValue(
        name,
        0
    );
    if (value == 0) {
        return 0;
    }
    return strcmp(
        value,
        "TRUE"
    ) == 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-setmacro
 * @recoil-artifact defines .text recoil:function:0x4c1780: zInterp_Context::SetMacro.
 *
 * Purpose: create or update one dynamically allocated macro table entry.
 */
int zInterp_Context::SetMacro(
    const char *name,
    const char *value
) {
    zInterp_MacroEntry *entry = 0;
    if (name != 0 && value != 0) {
        if (FindMacroValue(
            name,
            &entry
        ) != 0) {
            const size_t valueSize = strlen(value) + 1;
            entry->value = (char *)(realloc(
                entry->value,
                valueSize
            ));
            strcpy(entry->value, value);
            return 1;
        }

        macroTable =
            (zInterp_MacroEntry *)(realloc(
                macroTable,
                (macroCount + 1) * sizeof(zInterp_MacroEntry)
            ));
        entry = &macroTable[macroCount];
        entry->name = _strdup(name);
        entry->value = _strdup(value);
        ++macroCount;
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-echotokens
 * @recoil-artifact defines .text recoil:function:0x4c1870: zInterp_Context::EchoTokens.
 *
 * Purpose: print each parsed token followed by a newline.
 */
int zInterp_Context::EchoTokens() {
    for (unsigned int tokenIndex = 0; tokenIndex < tokenCount; ++tokenIndex) {
        printf(
            g_zInterp_PrintTokenWithSpaceFmt,
            tokenIndex < tokenCount ? tokenList[tokenIndex] : 0
        );
    }

    return printf("\n");
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-pushfileframe
 * @recoil-artifact defines .text recoil:function:0x4c18c0: zInterp_Context::PushFileFrame.
 *
 * Purpose: append one saved script file position for nested source commands.
 */
int zInterp_Context::PushFileFrame(
    FILE *file,
    long filePos,
    int hasPreparedInput
) {
    zInterp_FileFrame *const frames = (zInterp_FileFrame *)(realloc(
        fileFrameStack,
        (fileFrameCount + 1) * sizeof(zInterp_FileFrame)
    ));
    const int frameIndex = fileFrameCount;
    fileFrameStack = frames;

    frames[frameIndex].file = file;
    fileFrameStack[fileFrameCount].filePos = filePos;
    fileFrameStack[fileFrameCount].hasPreparedInput = hasPreparedInput;
    ++fileFrameCount;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-popfileframe
 * @recoil-artifact defines .text recoil:function:0x4c1940: zInterp_Context::PopFileFrame.
 *
 * Purpose: pop the most recent nested-script file frame without freeing storage.
 */
zInterp_FileFrame * zInterp_Context::PopFileFrame() {
    int count = fileFrameCount;
    if (count == 0) {
        return 0;
    }

    --count;
    fileFrameCount = count;
    return &fileFrameStack[count];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-clearfileframestack
 * @recoil-artifact defines .text recoil:function:0x4c1960: zInterp_Context::ClearFileFrameStack.
 *
 * Purpose: free saved nested-script file frames and reset the frame count.
 */
void zInterp_Context::ClearFileFrameStack() {
    if (fileFrameStack != 0) {
        free(fileFrameStack);
        fileFrameStack = 0;
    }
    fileFrameCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-nexttoken
 * @recoil-artifact defines .text recoil:function:0x4c1990: zInterp_Context::NextToken.
 *
 * Purpose: advance the token cursor and return the macro-expanded token text.
 */
char * zInterp_Context::NextToken() {
    const unsigned int tokenIndex = (unsigned int)(tokenReadIndex);
    tokenReadIndex = (int)(tokenIndex + 1);

    char *token = tokenIndex < tokenCount ? tokenList[tokenIndex] : 0;

    if (token == 0) {
        return 0;
    }

    return ExpandMacroRefs(token);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-parsebooltoken
 * @recoil-artifact defines .text recoil:function:0x4c19c0: zInterp_Context::ParseBoolToken.
 *
 * Purpose: parse the next token as an on/true boolean value.
 */
int zInterp_Context::ParseBoolToken() {
    char *const token = NextToken();
    if (token != 0) {
        if (_stricmp(token, "on") == 0 || _stricmp(token, "true") == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-parsefloattoken
 * @recoil-artifact defines .text recoil:function:0x4c1a00: zInterp_Context::ParseFloatToken.
 *
 * Purpose: parse the next token as a floating-point value.
 */
float zInterp_Context::ParseFloatToken() {
    char *const token = NextToken();
    if (token != 0) {
        return (float)(atof(token));
    }

    return 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-parseinttoken
 * @recoil-artifact defines .text recoil:function:0x4c1a20: zInterp_Context::ParseIntToken.
 *
 * Purpose: parse the next token as an integer value.
 */
int zInterp_Context::ParseIntToken() {
    char *const token = NextToken();
    if (token != 0) {
        return atoi(token);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-findvarentry
 * @recoil-artifact defines .text recoil:function:0x4c1a40: zInterp_Context::FindVarEntry.
 *
 * Purpose: find a registered script variable entry by name.
 */
zInterp_VarEntry * zInterp_Context::FindVarEntry(
    const char *name
) {
    zInterp_VarEntry *entry = varTable;
    for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex, ++entry) {
        if (strcmp(
            name,
            entry->name
        ) == 0) {
            return entry;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-dumpvarentry
 * @recoil-artifact defines .text recoil:function:0x4c1ab0: zInterp_Context::DumpVarEntry.
 *
 * Purpose: log one variable entry according to its stored scalar/string type.
 */
void zInterp_Context::DumpVarEntry(
    zInterp_VarEntry *entry
) {
    if (entry != 0) {
        switch (entry->type) {
        case 0:
            Logf(this, k_zInterp_FormatVarInt, entry->name, *entry->valuePtr.intPtr);
            break;
        case 1:
            Logf(this, k_zInterp_FormatVarFloat, entry->name, *entry->valuePtr.floatPtr);
            break;
        case 2:
            // Retail sign-extends *charPtr before passing it to the %s format.
            Logf(this, k_zInterp_FormatVarString, entry->name, *entry->valuePtr.charPtr);
            break;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-incerrorcount
 * @recoil-artifact defines .text recoil:function:0x4c1b20: zInterp_Context::IncErrorCount.
 *
 * Purpose: count one parser error for the current command line.
 */
void zInterp_Context::IncErrorCount() {
    ++errorCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-logf
 * @recoil-artifact defines .text recoil:function:0x4c1b30: zInterp_Context::Logf.
 *
 * Purpose: forward formatted parser logging to the context callback.
 */
void zInterp_Context::Logf(
    zInterp_Context *ctx,
    const char *fmt,
    ...
) {
    if (ctx->logFn != 0) {
        va_list args;
        va_start(
            args,
            fmt
        );
        ctx->logFn(
            fmt,
            (char *)args
        );
        va_end(args);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-evalconditionexpr
 * @recoil-artifact defines .text recoil:function:0x4c1b50: zInterp_Context::EvalConditionExpr.
 *
 * Purpose: evaluate simple macro truth expressions used by ifdef/ifndef.
 */
int zInterp_Context::EvalConditionExpr() {
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
        const unsigned int nameIndex = tokenIndex++;
        const char *const name = nameIndex < tokenCount ? tokenList[nameIndex] : 0;

        switch (op) {
        case 0:
            result = IsMacroTrue(name);
            break;
        case 1:
            result |= IsMacroTrue(name);
            break;
        case 2:
            result &= IsMacroTrue(name);
            break;
        }

        if (tokenIndex < tokenCount) {
            const unsigned int opIndex = tokenIndex++;
            const char *const opText = opIndex < tokenCount ? tokenList[opIndex] : 0;
            if (strncmp(
                opText,
                "||",
                2
            ) == 0) {
                op = 1;
            } else if (strncmp(
                opText,
                "&&",
                2
            ) == 0) {
                op = 2;
            } else {
                break;
            }
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-handlebuiltincommand
 * @recoil-artifact defines .text recoil:function:0x4c1c50: zInterp_Context::HandleBuiltinCommand.
 *
 * Purpose: handle parser builtins for conditions, macros, script inclusion,
 * and variable mutation before core command dispatch.
 */
int zInterp_Context::HandleBuiltinCommand(
    char *commandToken
) {
    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "endif",
        5
    ) == 0) {
        if (conditionalDepth != 0) {
            --conditionalDepth;
        }
        return 0;
    } else if (conditionalDepth != 0) {
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "ifdef",
        5
    ) == 0) {
        if (EvalConditionExpr() == 0) {
            ++conditionalDepth;
        }
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "ifndef",
        6
    ) == 0) {
        if (EvalConditionExpr() != 0) {
            ++conditionalDepth;
        }
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "mkdir",
        5
    ) == 0) {
        char *const path = NextToken();
        if (_mkdir(path) != 0 && errno != EEXIST) {
            ReportErrorf(
                this,
                "%s %s FAILED (errno == %d)",
                commandToken,
                path,
                errno
            );
        }
        return 0;
    }

    if (strcmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "Quit"
    ) == 0) {
        parseResult = 1;
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "set",
        3
    ) == 0) {
        char *const name = NextToken();
        char *const value = NextToken();
        SetMacro(
            name,
            value
        );
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "source",
        6
    ) == 0) {
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

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "who",
        3
    ) == 0) {
        Logf(
            this,
            "%d Macros",
            macroCount
        );
        for (unsigned int i = 0; i < macroCount; ++i) {
            Logf(
                this,
                "%12s: %s",
                macroTable[i].name,
                macroTable[i].value
            );
        }
        Logf(
            this,
            "%d Variables",
            varCount
        );
        zInterp_VarEntry *entry = varTable;
        for (unsigned int varIndex = 0; varIndex < varCount; ++varIndex, ++entry) {
            DumpVarEntry(entry);
        }
        return 0;
    }

    if (strncmp(
        tokenCount > 0 ? tokenList[0] : 0,
        "var",
        3
    ) == 0) {
        char *const name = NextToken();
        zInterp_VarEntry *const entry = FindVarEntry(name);
        if (entry == 0) {
            Logf(
                this,
                "Can't find variable ( %s )",
                name
            );
            return 0;
        }

        char *const op = NextToken();
        if (strncmp(
            op,
            "set",
            3
        ) != 0 && strncmp(
            op,
            &g_zInterp_AssignToken_Equal,
            1
        ) != 0) {
            IncErrorCount();
            return 0;
        }

        switch (entry->type) {
        case 0:
            *entry->valuePtr.intPtr = ParseIntToken();
            break;
        case 1:
            *entry->valuePtr.floatPtr = ParseFloatToken();
            break;
        case 2:
            Logf(this, "Can't modify string variables (yet!)");
            break;
        }
        DumpVarEntry(entry);
        return 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-printnodetree
 * @recoil-artifact defines .text recoil:function:0x4c2030: zInterp_Context::PrintNodeTree.
 *
 * Purpose: recursively log a zClass node tree with two-space child indentation.
 */
void zInterp_Context::PrintNodeTree(
    zClass_NodePartial *node,
    int indent
) {
    if (node != 0) {
        Logf(this, k_zInterp_PrintNodeTreeFormat, indent, " ", node->name);
        for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
            PrintNodeTree(node->listB[childIndex], indent + 2);
        }
    }
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-reportparseerror
 * @recoil-artifact defines .text recoil:function:0x4c2090: zInterp_Context::ReportParseError.
 *
 * Purpose: count an unhandled command parse error and report failure.
 */
int zInterp_Context::ReportParseError(
    char *
) {
    IncErrorCount();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-dispatchcorecommand
 * @recoil-artifact defines .text recoil:function:0x4c20a0: zInterp_Context::DispatchCoreCommand.
 *
 * Purpose: dispatch engine-facing script commands across zClass, zVideo,
 * zEffect, zModel, zRndr, zWeapon, and support subsystems.
 */
int zInterp_Context::DispatchCoreCommand(
    char *commandToken
) {
    float y;
    float x;
    float z;
    float red;
    float green;
    float blue;
    int excludedVertexIndices[2] = { 0, 1 };

    switch (commandToken[0]) {
    case 'A':
        if (CommandIs(
            this,
            "AddChild"
        ) != 0) {
            char *const searchName = NextToken();
            zClass_NodePartial *const child = zClass::FindByTypeAndName(
                6,
                searchName
            );
            zClass_NodePartial *const parent = (zClass_NodePartial *)(currentNode);
            if (parent == 0) {
                ReportErrorf(
                    this,
                    "%s %s FAILED because current node is NULL",
                    commandToken,
                    searchName
                );
                return 1;
            }
            if (child == 0) {
                ReportErrorf(
                    this,
                    "%s %s FAILED because node [%s] wasn't found",
                    commandToken,
                    searchName,
                    searchName
                );
                return 1;
            }
            zClass_Class::AddChild(
                parent,
                child
            );
            return 1;
        }

        if (CommandIs(
            this,
            "AddEnhancerImage"
        ) != 0) {
            zImage::TexDir_FindOrAppendByPath(NextToken());
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "AnimSetZBDFile"
        ) != 0) {
            zEffect_Anim::SetZbdFilename(NextToken());
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "AnimSetDebugFrame"
        ) != 0) {
            zEffect::SetAnimDebugFrameTag();
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'C':
        if (CommandIs(
            this,
            "CameraRotate"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Camera::gwCameraSetPosition(
                (zClass_NodePartial *)(currentNode),
                x,
                y,
                z
            );
            return 1;
        }

        if (CommandIsExact(
            this,
            "CameraGetTranslate"
        ) != 0) {
            if (currentNode == 0) {
                return 1;
            }

            zClass_Camera::gwCameraGetTarget(
                (zClass_NodePartial *)(currentNode),
                &x,
                &y,
                &z
            );
            Logf(
                this,
                "%s --> ( %.2f %.2f %.2f )",
                ((zClass_NodePartial *)(currentNode))->name,
                x,
                y,
                z
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetActive"
        ) != 0) {
            zClass_Camera::gwCameraSetActive(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CameraSetDynamicLOD"
        ) != 0) {
            x = ParseFloatToken();
            zClass_Camera::SetViewDistance(
                1,
                x
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CameraSetFOV"
        ) != 0) {
            float horizontalFov = ParseFloatToken();
            float verticalFov = ParseFloatToken();
            horizontalFov = (float)(horizontalFov * kDegreesToRadians);
            verticalFov = (float)(verticalFov * kDegreesToRadians);
            zClass_Camera::gwCameraSetFOV(
                (zClass_NodePartial *)(currentNode),
                horizontalFov,
                verticalFov
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetHorizonXZ"
        ) != 0) {
            zClass_NodePartial *const horizon = zClass::FindByTypeAndName(
                6,
                NextToken()
            );
            zClass_Camera::gwCameraSetHorizonXZ(
                (zClass_NodePartial *)(currentNode),
                horizon
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetHorizon"
        ) != 0) {
            zClass_NodePartial *const horizon = zClass::FindByTypeAndName(
                6,
                NextToken()
            );
            zClass_Camera::gwCameraSetHorizon(
                (zClass_NodePartial *)(currentNode),
                horizon
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CameraSetLODMultiplier"
        ) != 0) {
            const float clipDistance = ParseFloatToken();
            zClass_Camera::gwCameraSetClipDistance(
                (zClass_NodePartial *)(currentNode),
                clipDistance
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetNearFarClip"
        ) != 0) {
            const float nearClip = ParseFloatToken();
            const float farClip = ParseFloatToken();
            zClass_Camera::gwCameraSetNearFarClip(
                (zClass_NodePartial *)(currentNode),
                nearClip,
                farClip
            );
            return 1;
        }

        if (CommandIsExact(
            this,
            "CameraSetNearClip"
        ) != 0) {
            float nearClip;
            float farClip;
            zClass_Camera::gwCameraGetNearFarClip(
                (zClass_NodePartial *)(currentNode),
                &nearClip,
                &farClip
            );
            nearClip = ParseFloatToken();
            zClass_Camera::gwCameraSetNearFarClip(
                (zClass_NodePartial *)(currentNode),
                nearClip,
                farClip
            );
            return 1;
        }

        if (CommandIsExact(
            this,
            "CameraSetFarClip"
        ) != 0) {
            float nearClip;
            float farClip;
            zClass_Camera::gwCameraGetNearFarClip(
                (zClass_NodePartial *)(currentNode),
                &nearClip,
                &farClip
            );
            farClip = ParseFloatToken();
            zClass_Camera::gwCameraSetNearFarClip(
                (zClass_NodePartial *)(currentNode),
                nearClip,
                farClip
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CameraSetObjectHSETest"
        ) != 0) {
            zClass_Camera::SetObjectHseTestEnabled(ParseBoolToken());
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetWindow"
        ) != 0) {
            zClass_NodePartial *const window = zClass::FindByTypeAndName(
                14,
                NextToken()
            );
            zClass_Camera::gwCameraSetWindow(
                (zClass_NodePartial *)(currentNode),
                window
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraSetWorld"
        ) != 0) {
            zClass_NodePartial *const world = zClass::FindByTypeAndName(
                13,
                NextToken()
            );
            zClass_Camera::gwCameraSetWorld(
                (zClass_NodePartial *)(currentNode),
                world
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CameraTranslate"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Camera::gwCameraSetTarget(
                (zClass_NodePartial *)(currentNode),
                x,
                y,
                z
            );
            return 1;
        }

        if (strncmp(
            tokenCount > 0 ? tokenList[0] : 0,
            "CountCameraNodes",
            14
        ) == 0) {
            printf(
                "# of nodes in camera list = %d\n",
                zClass_TypeList::CountNodes(8)
            );
            return 1;
        }

        if (CommandIs(
            this,
            "CountUsedNodes"
        ) != 0) {
            printf(
                "# of nodes in used list = %d\n",
                zClass_TypeList::CountNodes(6)
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CycleTextureSetLooping"
        ) != 0) {
            zModel_Instance::SetCycleTextureLoop(
                g_zInterp_CurrentCycleTextureDi,
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CycleTextureSetMap"
        ) != 0) {
            zImage_TexDirEntryPartial *const texDirEntry =
                zImage::TexDir_FindOrAppendByPath(NextToken());
            zModel_Instance::AddCycleTexture(
                g_zInterp_CurrentCycleTextureDi,
                texDirEntry
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CycleTextureSetOn"
        ) != 0) {
            const int textureCount = ParseIntToken();
            if (currentNode == 0) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x198,
                    "No current node to enable cycle textures.  Take note of preceding "
                    "\"FindNode\" Error"
                );
                return 1;
            }

            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            if (g_zInterp_CurrentCycleTextureDi == 0) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x1a2,
                    "ERROR no GFX data for cycled texture (%s)",
                    ((zClass_NodePartial *)(currentNode))->name
                );
            }

            if (zDi::SetCurrentVariantCycleTextureCount(
                    g_zInterp_CurrentCycleTextureDi,
                    textureCount
                ) != 0) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x1a8,
                    "Node (%s) has no graphics data for cycled texture\n",
                    ((zClass_NodePartial *)(currentNode))->name
                );
            }
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "CycleTextureSetSpeed"
        ) != 0) {
            x = ParseFloatToken();
            zDi::SetCurrentVariantCycleTextureSpeed(
                g_zInterp_CurrentCycleTextureDi,
                x
            );
            return 1;
        }

        if (CommandIs(
            this,
            "ClearScreenBuffer"
        ) == 0) {
            IncErrorCount();
            return 1;
        }
        zVideo::ExchangeClearScreenBufferEnabled(ParseBoolToken());
        return 1;
    case 'D':
        if (CommandIs(
            this,
            "DeleteChild"
        ) != 0) {
            char *const name = NextToken();
            zClass_NodePartial *const child = zClass_Class::FindSubNodeByName(
                (zClass_NodePartial *)(currentNode),
                name
            );
            if (currentNode != 0 && child != 0) {
                zClass_Class::RemoveChild(
                    (zClass_NodePartial *)(currentNode),
                    child
                );
            } else {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x1c6,
                    "interp: DeleteChild(%s, %s) --> NULL NODE",
                    currentNode != 0 ? ((zClass_NodePartial *)(currentNode))->name : "NULL",
                    name
                );
            }
            return 1;
        }

        if (CommandIs(
            this,
            "DeleteFile"
        ) != 0) {
            DeleteFileA(NextToken());
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "DeleteTree"
        ) != 0) {
            char *const searchName = NextToken();
            zClass_NodePartial *const node = zClass::FindByTypeAndName(
                6,
                searchName
            );
            if (node == 0) {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x1de,
                    "interp: DeleteTree (%s) --> NULL NODE",
                    searchName
                );
                return 1;
            }
            zClass_Util::DestroyNodeRecursive(node);
            return 1;
        }

        if (CommandIs(
            this,
            "DisplayOrigin"
        ) != 0) {
            const int x = ParseIntToken();
            const int y = ParseIntToken();
            zClass_Display::gwDisplaySetPosition(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandIs(
            this,
            "DisplayResolution"
        ) != 0) {
            const int width = ParseIntToken();
            const int height = ParseIntToken();
            zClass_Display::gwDisplaySetSize(
                (zClass_NodePartial *)(currentNode),
                width,
                height
            );
            return 1;
        }

        if (CommandIs(
            this,
            "DisplaySetClearColor"
        ) != 0) {
            red = ParseFloatToken();
            green = ParseFloatToken();
            blue = ParseFloatToken();
            zClass_Display::gwDisplaySetBackgroundColor(
                (zClass_NodePartial *)(currentNode),
                red,
                green,
                blue
            );
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'e':
        if (CommandIs(
            this,
            "echo"
        ) != 0) {
            EchoTokens();
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'E':
        if (CommandIs(
            this,
            "Echo"
        ) != 0) {
            EchoTokens();
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'F':
        if (CommandIs(
            this,
            "FindNode"
        ) != 0) {
            char *const searchName = NextToken();
            currentNode = zClass::FindByTypeAndName(
                6,
                searchName
            );
            if (currentNode == 0) {
                ReportErrorf(
                    this,
                    "FindNode %s: FAILED",
                    searchName
                );
            }
            return 1;
        }

        if (CommandIs(
            this,
            "FindSubNode"
        ) != 0) {
            char *const name = NextToken();
            currentNode = zClass_Class::FindSubNodeByName(
                (zClass_NodePartial *)(currentNode),
                name
            );
            if (currentNode == 0) {
                ReportErrorf(
                    this,
                    "FindSubNode %s: FAILED",
                    name
                );
            }
            return 1;
        }

        if (CommandIs(
            this,
            "FreeNode"
        ) != 0) {
            char *const searchName = NextToken();
            zClass_NodePartial *const node = zClass::FindByTypeAndName(
                6,
                searchName
            );
            int result;
            switch (node->classId) {
                case 1:
                    result = zClass_Camera::DeleteNode(node);
                    break;
                case 2:
                    result = zClass_World::DeleteNode(node);
                    break;
                case 3:
                    result = zClass_Window::DeleteNode(node);
                    break;
                case 5:
                    result = zClass_Object3D::DeleteNode(node);
                    break;
                default:
                    printf(
                        "Unrecognized node class = %d\n",
                        node->classId
                    );
                    result = 1;
                    break;
            }
            if (result != 0) {
                printf(
                    "   Error freeing node %s\n",
                    searchName
                );
            }
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'G':
        if (CommandIs(
            this,
            "GameZReadZBDFile"
        ) != 0) {
            char *const filename = NextToken();
            if (GameZ::ReadZBDFile(filename) != 0) {
                ReportErrorf(
                    this,
                    "%s %s FAILED",
                    commandToken,
                    filename
                );
            }
            return 1;
        }

        if (CommandIs(
            this,
            "GameZWriteZBDFile"
        ) != 0) {
            char *const filename = NextToken();
            zClass_Class::gwNodeUpdateAll();
            zClass::ProcessDeferredWork();
            GameZ::WriteZBDFile(filename);
            return 1;
        }

        if (CommandIs(
            this,
            "GetBFETolerance"
        ) != 0) {
            Logf(
                this,
                "BFE Tolerance: %.7f",
                zModel::GetBackfaceEliminationToleranceScalar()
            );
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'I':
        return 1;
    case 'L':
        if (CommandIs(
            this,
            "LensFlareTexture"
        ) != 0) {
            const int stageIndex = ParseIntToken();
            zImage_TexDirEntryPartial *const texDirEntry =
                zImage::TexDir_FindOrAppendByPath(NextToken());
            zRndr_LensFlare_SetVisibleSampleStage(
                stageIndex,
                texDirEntry
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightNew"
        ) != 0) {
            currentNode = zClass_Light::gwLightNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetActive"
        ) != 0) {
            zClass_Class::gwNodeSetActive(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetAmbient"
        ) != 0) {
            x = ParseFloatToken();
            zClass_Light::gwLightSetIntensity(
                (zClass_NodePartial *)(currentNode),
                x
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetColor"
        ) != 0) {
            red = ParseFloatToken();
            green = ParseFloatToken();
            blue = ParseFloatToken();
            zClass_Light::gwLightSetSpecularColor(
                (zClass_NodePartial *)(currentNode),
                red,
                green,
                blue
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetDiffuse"
        ) != 0) {
            x = ParseFloatToken();
            zClass_Light::gwLightSetFalloff(
                (zClass_NodePartial *)(currentNode),
                x
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetDirectedSource"
        ) != 0) {
            zClass_Light::gwLightSetPointMode((zClass_NodePartial *)(currentNode));
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetDirectional"
        ) != 0) {
            zClass_Light::gwLightSetConeAngle(
                (zClass_NodePartial *)(currentNode),
                (unsigned int)(ParseBoolToken())
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetOrientation"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Light::gwLightSetRotation(
                (zClass_NodePartial *)(currentNode),
                (float)(x * kDegreesToRadians),
                (float)(y * kDegreesToRadians),
                (float)(z * kDegreesToRadians)
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetPointSource"
        ) != 0) {
            zClass_Light::gwLightSetDirectionalMode((zClass_NodePartial *)(currentNode));
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetRanges"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_Light::gwLightSetRange(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (strncmp(
            tokenCount > 0 ? tokenList[0] : 0,
            "LightSetSaturated",
            19
        ) == 0) {
            if (currentNode == 0) {
                ReportErrorf(
                    this,
                    "%s Failed: no current node"
                );
                return 1;
            }

            zClass_Light::gwLightSetParam(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LightSetTranslate"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Light::gwLightSetPosition(
                (zClass_NodePartial *)(currentNode),
                x,
                y,
                z
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LoadSoils"
        ) != 0) {
            zRndr_GlobalStringTable::LoadDynamicEntriesFromPath(NextToken());
            return 1;
        }

        if (CommandIs(
            this,
            "LODAddChild"
        ) != 0) {
            zClass_NodePartial *const child = zClass::FindByTypeAndName(
                6,
                NextToken()
            );
            zClass_Lod::gwLodAddChild(
                (zClass_NodePartial *)(currentNode),
                child
            );
            return 1;
        }

        if (CommandIs(
            this,
            "LODSetRange"
        ) != 0) {
            float nearRange = ParseFloatToken();
            float farRange = ParseFloatToken();
            ((zClass_LodDataPartial *)(((zClass_NodePartial *)(currentNode))->classData))->nearRangeSq = nearRange * nearRange;
            ((zClass_LodDataPartial *)(((zClass_NodePartial *)(currentNode))->classData))->farRangeSq = farRange * farRange;
        } else {
            IncErrorCount();
        }
        return 1;
    case 'm':
    case 'M':
        if (CommandIs(
            this,
            "MatlFaceColor"
        ) != 0) {
            runtimeBlob->material.colorRgb.red = ParseFloatToken();
            runtimeBlob->material.colorRgb.green = ParseFloatToken();
            runtimeBlob->material.colorRgb.blue = ParseFloatToken();
            runtimeBlob->material.packedColor =
                zVid_PackColorRgbFloats((zVideo_ColorRgbFloat *)(&runtimeBlob->material.colorRgb));
            return 1;
        }

        if (CommandIs(
            this,
            "MatlNew"
        ) != 0) {
            zModel_Material::ResetDefaults(&runtimeBlob->material);
            return 1;
        }

        if (CommandIs(
            this,
            "MatlTexture"
        ) != 0) {
            runtimeBlob->material.currentTextureDirectoryEntry =
                zImage::FindTexDirEntryByName(NextToken());
            if (runtimeBlob->material.currentTextureDirectoryEntry == 0) {
                runtimeBlob->material.flags &= 0xfeff;
            } else {
                runtimeBlob->material.flags |= 0x0100;
            }
            return 1;
        }

        if (CommandIs(
            this,
            "ModelNew"
        ) != 0) {
            currentNode = zClass_Object3D::gwObject3DInit();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            runtimeBlob->displayInstance = zModel_DiPool::AllocFromFreeList();
            zClass_Class::gwNodeSetDisplayInstance(
                (zClass_NodePartial *)(currentNode),
                runtimeBlob->displayInstance
            );
            const char *const modelType = NextToken();
            if (strncmp(
                modelType,
                "Facade",
                6
            ) == 0) {
                zUtil::StoreInt32(
                    (int *)runtimeBlob->displayInstance,
                    1
                );
            } else {
                zUtil::StoreInt32(
                    (int *)runtimeBlob->displayInstance,
                    0
                );
            }
            return 1;
        }

        if (CommandIs(
            this,
            "ModelPolygonBegin"
        ) != 0) {
            runtimeBlob->pointCount = 0;
            runtimeBlob->uvCount = 0;
            runtimeBlob->normalsA = 0;
            return 1;
        }

        if (CommandIs(
            this,
            "ModelPolygonEnd"
        ) != 0) {
            runtimeBlob->polygonMaterial = zModel_Material::FindOrClone(&runtimeBlob->material);
            zTag4::Clear(&runtimeBlob->variantTag);
            zDi::AddPolygon(
                runtimeBlob->displayInstance,
                runtimeBlob->pointCount,
                runtimeBlob->polygonPoints,
                runtimeBlob->uvPairs,
                runtimeBlob->normalsA,
                runtimeBlob->normalsB,
                runtimeBlob->secondaryUvPairs,
                runtimeBlob->polygonMaterial,
                runtimeBlob->drawFlags,
                runtimeBlob->flagBit8,
                &runtimeBlob->variantTagWord
            );
            return 1;
        }

        if (CommandIs(
            this,
            "ModelPolygonUV"
        ) != 0) {
            const int uvIndex = runtimeBlob->uvCount;
            runtimeBlob->uvPairs[uvIndex].u = ParseFloatToken();
            runtimeBlob->uvPairs[uvIndex].v = ParseFloatToken();
            ++runtimeBlob->uvCount;
            return 1;
        }

        if (CommandIs(
            this,
            "ModelPolygonVertex"
        ) != 0) {
            const int pointIndex = runtimeBlob->pointCount;
            runtimeBlob->polygonPoints[pointIndex].x = ParseFloatToken();
            runtimeBlob->polygonPoints[pointIndex].y = ParseFloatToken();
            runtimeBlob->polygonPoints[pointIndex].z = ParseFloatToken();
            ++runtimeBlob->pointCount;
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'N':
        if (CommandIs(
            this,
            "NewCamera"
        ) != 0) {
            currentNode = zClass_Camera::gwCameraNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewDisplay"
        ) != 0) {
            currentNode = zClass_Display::gwDisplayInit();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewLOD"
        ) != 0) {
            currentNode = zClass_Lod::gwLodNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewNode"
        ) != 0) {
            currentNode = zClass_Class::AllocNodeFromFreeList();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewObject3D"
        ) != 0) {
            currentNode = zClass_Object3D::gwObject3DInit();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewSEQ"
        ) != 0) {
            currentNode = zClass_Sequence::gwSequenceNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewWindow"
        ) != 0) {
            currentNode = zClass_Window::gwWindowNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NewWorld"
        ) != 0) {
            currentNode = zClass_World::gwWorldNew();
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NodeSetActive"
        ) != 0) {
            zClass_Class::gwNodeSetActive(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NodeSetDescription"
        ) != 0) {
            zClass_Class::gwNodeSetName(
                (zClass_NodePartial *)(currentNode),
                NextToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NodeSetCanModify"
        ) != 0) {
            zClass_Class::gwNodeSetFlag16(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NodeSetLighting"
        ) != 0) {
            zClass_Node::AssignInt32ToDiRecursive(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "NodeSetOverwrite"
        ) != 0) {
            zClass_Class::gwNodeSetVertexAlphaOverride(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'O':
        if (CommandIs(
            this,
            "Object3DAddChild"
        ) != 0) {
            char *const searchName = NextToken();
            zClass_NodePartial *const child = zClass::FindByTypeAndName(
                6,
                searchName
            );
            zClass_NodePartial *const parent = (zClass_NodePartial *)(currentNode);
            if (parent != 0 && child != 0) {
                zClass_Object3D::gwObject3DAddChild(
                    parent,
                    child
                );
            } else {
                zError::ReportOld(
                    0x200,
                    "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                    0x3f3,
                    "interp: Object3DAddChild(%s, %s) --> NULL NODE",
                    parent != 0 ? parent->name : "NULL",
                    searchName
                );
            }
            return 1;
        }

        if (CommandIsExact(
            this,
            "Object3DGetTranslate"
        ) != 0) {
            if (currentNode == 0) {
                return 1;
            }
            zClass_Object3D::gwObject3DGetPosition(
                (zClass_NodePartial *)(currentNode),
                &x,
                &y,
                &z
            );
            Logf(
                this,
                "%s --> ( %.2f %.2f %.2f )",
                ((zClass_NodePartial *)(currentNode))->name,
                x,
                y,
                z
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DRegisterTexturesToWorld"
        ) != 0) {
            const int registerTextures = ParseBoolToken();
            if (registerTextures != 0) {
                zClass_Class::gwNodeGetUserData(
                    (zClass_NodePartial *)(currentNode),
                    &g_zInterp_NodeUserDataScratch
                );
                g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
                g_zInterp_CurrentCycleTextureDi->flags |= 0x04;
            } else {
                zClass_Class::gwNodeGetUserData(
                    (zClass_NodePartial *)(currentNode),
                    &g_zInterp_NodeUserDataScratch
                );
                g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
                g_zInterp_CurrentCycleTextureDi->flags &= ~0x04;
            }
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DRotate"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Object3D::gwObject3DSetRotation(
                (zClass_NodePartial *)(currentNode),
                (float)(x * kDegreesToRadians),
                (float)(y * kDegreesToRadians),
                (float)(z * kDegreesToRadians)
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DScale"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Object3D::gwObject3DSetScale(
                (zClass_NodePartial *)(currentNode),
                x,
                y,
                z
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetActionPriority"
        ) != 0) {
            zClass_Class::gwNodeSetPriority(
                (zClass_NodePartial *)(currentNode),
                ParseIntToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetActive"
        ) != 0) {
            zClass_Class::gwNodeSetActive(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetColor"
        ) != 0) {
            const int colorMode = ParseIntToken();
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_Object3DCommandDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            zDi::SetObject3DColorModeForMaterials(
                g_zInterp_Object3DCommandDi,
                colorMode
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetFacade"
        ) != 0) {
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            zUtil::StoreInt32(
                (int *)g_zInterp_CurrentCycleTextureDi,
                1
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetOpacityIsSet"
        ) != 0) {
            zClass_Object3D::gwObject3DSetLitFlag(
                (zClass_NodePartial *)(currentNode),
                ParseIntToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetOpacity"
        ) != 0) {
            const float opacity = ParseFloatToken();
            zClass_Object3D::gwObject3DSetAlphaScale(
                (zClass_NodePartial *)(currentNode),
                opacity
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetPoints"
        ) != 0) {
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_CurrentCycleTextureDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            zUtil::StoreInt32(
                (int *)g_zInterp_CurrentCycleTextureDi,
                2
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetPriority"
        ) != 0) {
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_Object3DCommandDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            zDi::SetEntryValueForAllEntries(
                g_zInterp_Object3DCommandDi,
                (unsigned int)(ParseIntToken())
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetScrollAlways"
        ) != 0) {
            const int enabled = ParseBoolToken();
            x = ParseFloatToken();
            y = ParseFloatToken();
            if (enabled != 0) {
                if (RegisterScrollAlwaysNode(
                        (zClass_NodePartial *)(currentNode),
                        x,
                        y,
                        1
                    ) == 0) {
                    zError::ReportOld(
                        0x200,
                        "D:\\Proj\\GameZRecoil\\zInterp\\zinterp_parse.cpp",
                        0x462,
                        "Object3DSetScrollAlways on: FAILED  (node=0x%08x) (gfx=0x%08x)",
                        currentNode,
                        g_zInterp_CurrentCycleTextureDi
                    );
                }
                return 1;
            }
            DefaultDispatchHook((zClass_NodePartial *)(currentNode));
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetScroll"
        ) != 0) {
            const int enabled = ParseBoolToken();
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_NodePartial *const node = (zClass_NodePartial *)(currentNode);
            if (node == 0) {
                ReportErrorf(
                    this,
                    "%s %s %.1f %.1f Failed: current_node is NULL",
                    commandToken,
                    enabled != 0 ? "ON" : g_zInterp_PrintNodeTree_OffString,
                    x,
                    y
                );
                return 1;
            }

            if (enabled != 0) {
                RegisterScrollAlwaysNode(
                    node,
                    x,
                    y,
                    0
                );
            } else {
                DefaultDispatchHook(node);
            }
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetShowBackFace"
        ) != 0) {
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            g_zInterp_Object3DCommandDi = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            zDi::SetShowBackFaceForAllEntries(
                g_zInterp_Object3DCommandDi,
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetTextureWorldBaseCoordinates"
        ) != 0) {
            x = ParseFloatToken();
            z = ParseFloatToken();
            zModel::SetTextureWorldBase(
                x,
                z
            );
            return 1;
        }

        if (strncmp(
            tokenCount > 0 ? tokenList[0] : 0,
            "Object3DSetTextureWorldTexturesPerMeter",
            37
        ) == 0) {
            x = ParseFloatToken();
            z = ParseFloatToken();
            zModel::SetTextureWorldPerMeter(
                x,
                z
            );
            return 1;
        }

        if (CommandIs(
            this,
            "Object3DSetMorphVertex"
        ) != 0) {
            y = ParseFloatToken();
            zClass_Class::gwNodeGetUserData(
                (zClass_NodePartial *)(currentNode),
                &g_zInterp_NodeUserDataScratch
            );
            zDiPartial *const di = (zDiPartial *)g_zInterp_NodeUserDataScratch;
            g_zInterp_CurrentCycleTextureDi = di;
            zDi::BuildBlendVertsFromConnectivity(
                di,
                excludedVertexIndices,
                y,
                0,
                6
            );
            g_zInterp_CurrentCycleTextureDi->blendScale = 1.0f;
        } else if (CommandIs(
            this,
            "Object3DTranslate"
        ) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            z = ParseFloatToken();
            zClass_Object3D::gwObject3DSetPosition(
                (zClass_NodePartial *)(currentNode),
                x,
                y,
                z
            );
        } else {
            IncErrorCount();
        }
        break;
    case 'P':
        if (CommandIs(
            this,
            "PerspectiveTexture"
        ) != 0) {
            zRndr::g_perspectiveTextureEnabled = ParseBoolToken();
            return 1;
        }

        if (CommandIs(
            this,
            "PrintNodeCount"
        ) != 0) {
            char *const prefixText = NextToken();
            zClass::FindNextByTypePrefix(
                prefixText,
                6
            );
            int count = 0;
            zClass_NodePartial *node = zClass::FindNextByTypePrefix(
                0,
                0
            );
            while (node != 0) {
                ++count;
                node = zClass::FindNextByTypePrefix(
                    0,
                    0
                );
            }
            printf(
                "Node count for %s = %d\n",
                prefixText,
                count
            );
            return 1;
        }

        if (CommandIsExact(
            this,
            "PrintTree"
        ) != 0) {
            if (currentNode == 0) {
                ReportErrorf(
                    this,
                    "No current node"
                );
            }
            PrintNodeTree(
                (zClass_NodePartial *)(currentNode),
                2
            );
            return 1;
        }

        if (CommandIs(
            this,
            "PrintUsedNodes"
        ) != 0) {
            zClass_TypeList::PrintBucket(6);
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'R':
        if (CommandIs(
            this,
            "RdrAddPath"
        ) != 0) {
            zUtil_ZRDR_AppendSearchPath(NextToken());
            return 1;
        }

        if (CommandIs(
            this,
            "RdrSetPath"
        ) != 0) {
            zUtil_ZRDR_SetSearchPath(NextToken());
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'S': {
        if (CommandHasPrefix(
            this,
            "SEQ"
        ) != 0) {
            if (CommandHasPrefix(
                this,
                "SEQAddChild"
            ) != 0) {
                char *const searchName = NextToken();
                const int insertIndex = ParseIntToken();
                const float delay = ParseFloatToken();
                zClass_NodePartial *const child = zClass::FindByTypeAndName(
                    6,
                    searchName
                );
                zClass_Sequence::gwSequenceAddChild(
                    (zClass_NodePartial *)(currentNode),
                    child,
                    insertIndex,
                    delay
                );
                return 1;
            }

            if (CommandHasPrefix(
                this,
                "SEQNew"
            ) != 0) {
                currentNode = zClass_Sequence::gwSequenceNew();
                zClass_Class::gwNodeSetName(
                    (zClass_NodePartial *)(currentNode),
                    NextToken()
                );
                return 1;
            }

            if (CommandHasPrefix(
                this,
                "SEQSetActive"
            ) != 0) {
                zClass_Sequence::SetActive(
                    (zClass_NodePartial *)(currentNode),
                    ParseIntToken()
                );
                return 1;
            }

            if (CommandHasPrefix(
                this,
                "SEQSetLoop"
            ) != 0) {
                zClass_Sequence::SetLoop(
                    (zClass_NodePartial *)(currentNode),
                    ParseIntToken()
                );
                return 1;
            }

            if (strncmp(
                tokenCount > 0 ? tokenList[0] : 0,
                "SEQSetPause",
                12
            ) == 0) {
                zClass_Sequence::SetPause(
                    (zClass_NodePartial *)(currentNode),
                    ParseIntToken()
                );
                return 1;
            }

            if (CommandHasPrefix(
                this,
                "SEQSetRepeat"
            ) != 0) {
                zClass_Sequence::SetRepeat(
                    (zClass_NodePartial *)(currentNode),
                    ParseIntToken()
                );
            }
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "SetAltitudeSurface"
        ) != 0) {
            zClass_Class::gwNodeSetCellPickable(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "SetBFETolerance"
        ) != 0) {
            if (ValidateArgsAndNodeType(
                1,
                0,
                0
            ) != 0) {
                const float tolerance = ParseFloatToken();
                zModel::SetBackfaceEliminationToleranceScalar(tolerance);
            }
            return 1;
        }

        if (CommandIs(
            this,
            "SetCoplanarTolerance"
        ) != 0) {
            const float tolerance = ParseFloatToken();
            zModel_Const::SetCoplanarTolerance(tolerance);
            return 1;
        }

        if (CommandIs(
            this,
            "SetColinearTolerance"
        ) != 0) {
            const float tolerance = ParseFloatToken();
            zModel_Const::SetColinearTolerance(tolerance);
            return 1;
        }

        if (CommandIs(
            this,
            "SetGameZNodeArraySize"
        ) != 0) {
            zClass::SetNodeArraySize(ParseIntToken());
            return 1;
        }

        if (CommandIs(
            this,
            "SetMaterialArraySize"
        ) != 0) {
            zModel_MatlBuffer::SetArraySize(ParseIntToken());
            return 1;
        }

        if (CommandIs(
            this,
            "SetModel3DArraySize"
        ) != 0) {
            zModel::SetDisplayInstancePoolCapacity(ParseIntToken());
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "SetIntersectBBOX"
        ) != 0) {
            zClass_Class::gwNodeSetPickable(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "SetIntersectSurface"
        ) != 0) {
            zClass_Class::gwNodeSetRaycastable(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "SetLandmark"
        ) != 0) {
            zClass_Class::gwNodeSetBypassFarClip(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandIs(
            this,
            "SetPaletteName"
        ) != 0) {
            zVideo::LoadPaletteFileAndApplyBrightness(NextToken());
            return 1;
        }

        if (CommandIs(
            this,
            "SetPaletteShading"
        ) != 0) {
            zModel::SetSoftwarePathActive(ParseBoolToken());
            return 1;
        }

        if (CommandIs(
            this,
            "SetPerspectiveAdaptiveCorrection"
        ) != 0) {
            const int minSpan = ParseIntToken();
            const int maxSpan = ParseIntToken();
            const float scale = ParseFloatToken();
            zRndr::SetPerspectiveAdaptiveSpanParams(
                minSpan,
                maxSpan,
                scale
            );
            return 1;
        }

        if (CommandIs(
            this,
            "SetPerspectiveTextureDeltaX"
        ) != 0) {
            zRndr::SetPerspectiveTextureDeltaX(ParseIntToken());
            return 1;
        }

        if (CommandIs(
            this,
            "SetInverseZTolerance"
        ) != 0) {
            const float tolerance = ParseFloatToken();
            zRndr::SetInverseZTolerance(tolerance);
            return 1;
        }

        if (strncmp(
            tokenCount > 0 ? tokenList[0] : 0,
            "SetPerspectiveInverseZTolerance",
            20
        ) == 0) {
            const float tolerance = ParseFloatToken();
            zRndr::SetPerspectiveAdaptiveCorrection(tolerance);
            return 1;
        }

        if (CommandIs(
            this,
            "SetPerspectiveTextureFarZ"
        ) != 0) {
            const float farZ = ParseFloatToken();
            zRndr::SetPerspectiveTextureFarZ(farZ);
            return 1;
        }

        if (CommandHasPrefix(
            this,
            "SetProximity"
        ) != 0) {
            zClass_Class::gwNodeSetHasHitCallback(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        const int matchesSmallPolygonRejectArea = strncmp(
            GetCurrentCommand(),
            "SetSmallPolygonRejectArea",
            0x19
        ) == 0;
        if (matchesSmallPolygonRejectArea != 0) {
            const float area = ParseFloatToken();
            zModel::UpdateSmallPolyRejectThresholds(area);
            return 1;
        }

        if (CommandEqualsPrefix(
            "SetTextureDirectory",
            0x13
        ) != 0) {
            zImage_InitMissionResources(NextToken());
            return 1;
        }

        if (CommandEqualsPrefix(
            "SetVertexShading",
            0x10
        ) != 0) {
            zModel::SetVertexShadingEnabled(ParseBoolToken());
            return 1;
        }

        IncErrorCount();
        return 1;
    }
    case 'T':
        if (CommandEqualsPrefix("TextureAdd", 0xa) != 0) {
            zImage::TexDir_FindOrAppendByPath(NextToken());
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'V':
        if (CommandEqualsPrefix("Verbose", 7) != 0) {
            g_zInterp_VerboseLevel = ParseBoolToken();
            return 1;
        }

        if (CommandEquals("VideoSetDither") != 0) {
            zVideo_dd3d::SetPendingDitherEnable(ParseBoolToken());
            return 1;
        }

        if (CommandEquals("VideoSetWireFrame") != 0) {
            zVideo_dd3d::SetPendingWireframeState(ParseBoolToken());
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    case 'W':
        if (CommandEqualsPrefix("WindowAddClearPolygonVertex", 0x1b) != 0) {
            if (ValidateArgsAndNodeType(
                3,
                3,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                zVec3 point;
                point.x = ParseFloatToken();
                point.y = ParseFloatToken();
                point.z = ParseFloatToken();
                zClass_Window::gwWindowAddClearPolygonVertex(
                    (zClass_NodePartial *)(currentNode),
                    &point
                );
            }
            return 1;
        }

        if (CommandEqualsPrefix("WindowBuffer", 0xf) != 0) {
            if (ValidateArgsAndNodeType(
                1,
                3,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                zClass_Window::gwWindowSetBuffer(
                    (zClass_NodePartial *)(currentNode),
                    ParseIntToken()
                );
            }
            return 1;
        }

        if (CommandEqualsPrefix("WindowCloseClearPolygon", 0x17) != 0) {
            zClass_Window::gwWindowCloseClearPolygon((zClass_NodePartial *)(currentNode));
            return 1;
        }

        if (CommandEqualsPrefix("WindowOrigin", 0xc) != 0) {
            const int width = ParseIntToken();
            const int height = ParseIntToken();
            zClass_Window::gwWindowSetSize(
                (zClass_NodePartial *)(currentNode),
                width,
                height
            );
            return 1;
        }

        if (CommandEqualsPrefix("WindowResolution", 0x10) != 0) {
            const int width = ParseIntToken();
            const int height = ParseIntToken();
            zClass_Window::gwWindowSetResolution(
                (zClass_NodePartial *)(currentNode),
                width,
                height
            );
            return 1;
        }

        if (CommandEqualsPrefix("WindowSetClearPolygon", 0x15) != 0) {
            zClass_Window::gwWindowSetClearPolygon(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldAddLight", 0xd) != 0) {
            zClass_NodePartial *const light = zClass::FindByTypeAndName(
                9,
                NextToken()
            );
            zClass_World::AddLight(
                (zClass_NodePartial *)(currentNode),
                light
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldExtents", 0xc) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_World::gwWorldSetSize(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldOrigin", 0xb) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_World::gwWorldSetOrigin(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldPartitionInclusionTolerance", 0x20) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_World::gwWorldSetPartitionInclusionTolerance(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldPartitionMaxDECFeatureCount", 0x20) != 0) {
            zClass_World::gwWorldSetMaxDecFeatures(
                (zClass_NodePartial *)(currentNode),
                ParseIntToken()
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldPartition", 0xe) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_World::gwWorldSetVirtualAreaPartition(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldSetFogAltitude", 0x13) != 0) {
            x = ParseFloatToken();
            y = ParseFloatToken();
            zClass_World::SetPendingFogAltitudeRange(
                (zClass_NodePartial *)(currentNode),
                x,
                y
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldSetFogColor", 0x10) != 0) {
            red = ParseFloatToken();
            green = ParseFloatToken();
            blue = ParseFloatToken();
            zClass_World::SetPendingFogColorRgb01(
                (zClass_NodePartial *)(currentNode),
                red,
                green,
                blue
            );
            return 1;
        }

        if (CommandEqualsPrefix("WorldSetFogDensity", 0x12) != 0) {
            const float density = ParseFloatToken();
            zClass_World::SetPendingFogDensity(
                (zClass_NodePartial *)(currentNode),
                density
            );
            return 1;
        }

        if (CommandEquals("WorldSetFogRange") != 0) {
            if (ValidateArgsAndNodeType(
                2,
                2,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                const float nearRange = ParseFloatToken();
                const float farRange = ParseFloatToken();
                zClass_World::SetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    nearRange,
                    farRange
                );
            }
            return 1;
        }

        if (CommandEquals("WorldSetFogRangeNear") != 0) {
            if (ValidateArgsAndNodeType(
                1,
                2,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                float nearRange;
                float farRange;
                zClass_World::GetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    &nearRange,
                    &farRange
                );
                nearRange = ParseFloatToken();
                zClass_World::SetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    nearRange,
                    farRange
                );
            }
            return 1;
        }

        if (CommandEquals("WorldSetFogRangeFar") != 0) {
            if (ValidateArgsAndNodeType(
                1,
                2,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                float nearRange;
                float farRange;
                zClass_World::GetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    &nearRange,
                    &farRange
                );
                farRange = ParseFloatToken();
                zClass_World::SetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    nearRange,
                    farRange
                );
            }
            return 1;
        }

        if (CommandEquals("WorldGetFogRange") != 0) {
            if (ValidateArgsAndNodeType(
                0,
                2,
                (zClass_NodePartial *)(currentNode)
            ) != 0) {
                float nearRange;
                float farRange;
                zClass_World::GetPendingFogRange(
                    (zClass_NodePartial *)(currentNode),
                    &nearRange,
                    &farRange
                );
                Logf(
                    this,
                    "Fog Range: [%s] [ %.2f, %.2f ]",
                    ((zClass_NodePartial *)(currentNode))->name,
                    nearRange,
                    farRange
                );
            }
            return 1;
        }

        if (CommandEqualsPrefix("WorldSetFogState", 0x10) != 0) {
            if (ValidateArgsAndNodeType(
                1,
                2,
                (zClass_NodePartial *)(currentNode)
            ) == 0) {
                return 1;
            }

            char *const state = NextToken();
            if (strncmp(
                state,
                "linear",
                6
            ) == 0) {
                zClass_World::SetPendingFogState(
                    (zClass_NodePartial *)(currentNode),
                    1
                );
            } else if (strncmp(
                state,
                "exponential",
                11
            ) == 0) {
                zClass_World::SetPendingFogState(
                    (zClass_NodePartial *)(currentNode),
                    2
                );
            } else if (strncmp(
                state,
                "off",
                3
            ) == 0) {
                zClass_World::SetPendingFogState(
                    (zClass_NodePartial *)(currentNode),
                    0
                );
            } else {
                printf(
                    "Did not understand: %s\n",
                    state
                );
            }
            return 1;
        }

        if (CommandEqualsPrefix("WorldSetVirtualPartition", 0x18) != 0) {
            zClass_World::SetVirtualPartition(
                (zClass_NodePartial *)(currentNode),
                ParseBoolToken()
            );
            return 1;
        }

        if (CommandEqualsPrefix("WriteTextureSetType", 0x13) != 0) {
            OptCatalog::SetDamageMaskSlotIndex(ParseIntToken());
            return 1;
        }

        if (CommandEqualsPrefix("WriteTextureSetMap", 0x12) != 0) {
            OptCatalog::RegisterDamageMaskSlotPtr(zImage::TexDir_FindOrAppendByPath(NextToken()));
            return 1;
        } else {
            IncErrorCount();
            return 1;
        }
    default:
        IncErrorCount();
        break;
    }

    return 1;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-commandequalsprefix
 * @recoil-artifact defines .text recoil:function:0x4c5480: zInterp_Context::CommandEqualsPrefix.
 *
 * Purpose: compare the current command token against a caller-supplied prefix.
 */
int zInterp_Context::CommandEqualsPrefix(
    const char *prefix,
    unsigned int prefixLen
) {
    char *command = tokenCount > 0 ? tokenList[0] : 0;

    return strncmp(
        command,
        prefix,
        prefixLen
    ) == 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-commandequals
 * @recoil-artifact defines .text recoil:function:0x4c54b0: zInterp_Context::CommandEquals.
 *
 * Purpose: compare the current command token against a complete string.
 */
int zInterp_Context::CommandEquals(
    const char *other
) {
    char *command = tokenCount > 0 ? tokenList[0] : 0;

    return strcmp(
        command,
        other
    ) == 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-getcurrentcommand
 * @recoil-artifact defines .text recoil:function:0x4c5510: zInterp_Context::GetCurrentCommand.
 *
 * Purpose: return token zero for the current parsed command line.
 */
char * zInterp_Context::GetCurrentCommand() {
    if (tokenCount > 0) {
        return tokenList[0];
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-reporterrorf
 * @recoil-artifact defines .text recoil:function:0x4c5520: zInterp_Context::ReportErrorf.
 *
 * Purpose: mark the current line failed and forward formatted parser logging.
 */
void zInterp_Context::ReportErrorf(
    zInterp_Context *ctx,
    const char *fmt,
    ...
) {
    ctx->lineHadError = 1;
    if (ctx->logFn != 0) {
        va_list args;
        va_start(
            args,
            fmt
        );
        ctx->logFn(
            fmt,
            (char *)args
        );
        va_end(args);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-loadpreparedscriptindex
 * @recoil-artifact defines .text recoil:function:0x4c5550: zInterp_Context::LoadPreparedScriptIndex.
 *
 * Purpose: open and validate the prepared script index and cache its entries.
 */
int zInterp_Context::LoadPreparedScriptIndex(
    const char *zrdrPath
) {
    zInterp_PreparedScriptHeader preparedHeader;
    memset(&preparedHeader, 0, sizeof(preparedHeader));
    unsigned int preparedEntryCountValue = 0;

    if (preparedIndexStream != 0) {
        return 1;
    }

    if (archiveSearchList == 0) {
        archiveSearchList = zUtil_ZRDR_CreateSearchPathList(searchPathSpec);
    }

    preparedIndexStream = zUtil_ZRDR_OpenFileResolved(
        archiveSearchList,
        zrdrPath,
        "rb"
    );
    if (preparedIndexStream == 0) {
        return 0;
    }

    int headerRead = fread(
        &preparedHeader,
        sizeof(preparedHeader),
        1,
        preparedIndexStream
    ) == 1;
    if (!headerRead) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
    }
    if (!headerRead) {
        return 0;
    }

    zInterp_PreparedScriptEntry *entries = 0;
    int tableRead = 0;
    if (preparedHeader.magic == kPreparedScriptMagic) {
        int countRead = preparedHeader.version == kPreparedScriptVersion && fread(
            &preparedEntryCountValue,
            4,
            1,
            preparedIndexStream
        ) == 1;
        if (!countRead) {
            fclose(preparedIndexStream);
            preparedIndexStream = 0;
        }
        if (!countRead) {
            return 0;
        }

        entries = (zInterp_PreparedScriptEntry *)realloc(
            0,
            (preparedEntryCountValue + 1) *
                sizeof(zInterp_PreparedScriptEntry)
        );
        tableRead = entries != 0 && fread(
            entries,
            sizeof(zInterp_PreparedScriptEntry),
            preparedEntryCountValue,
            preparedIndexStream
        ) == preparedEntryCountValue;
    }
    if (!tableRead) {
        fclose(preparedIndexStream);
        preparedIndexStream = 0;
    }
    if (!tableRead) {
        return 0;
    }

    int entriesFresh = 1;
    for (int entryIndex = 0;
        entryIndex < (int)(preparedEntryCountValue) && entriesFresh != 0;
        ++entryIndex) {
        struct _stat sourceStat;
        if (_stat(entries[entryIndex].path, &sourceStat) == 0 &&
            entries[entryIndex].fileTime != sourceStat.st_mtime) {
            entriesFresh = 0;
        }
    }

    if (entriesFresh != 0) {
        preparedIndexHeader = preparedHeader;
        *preparedEntryCount = (int)(preparedEntryCountValue);
        preparedEntryTable = entries;
        return 1;
    }

    fclose(preparedIndexStream);
    preparedIndexStream = 0;
    free(entries);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-openpreparedscriptstream
 * @recoil-artifact defines .text recoil:function:0x4c5740: zInterp_Context::OpenPreparedScriptStream.
 *
 * Purpose: locate a prepared script entry and seek the shared stream to it.
 */
FILE * zInterp_Context::OpenPreparedScriptStream(
    const char *commandName
) {
    if (preparedIndexStream != 0) {
        int matchedIndex = -1;
        for (int entryIndex = 0; entryIndex < *preparedEntryCount; ++entryIndex) {
            if (_stricmp(
                preparedEntryTable[entryIndex].path,
                commandName
            ) == 0) {
                matchedIndex = entryIndex;
                break;
            }
        }
        if (matchedIndex != -1) {
            zInterp_PreparedScriptEntry *const matchedEntry = &preparedEntryTable[matchedIndex];
            int usePreparedStream = 1;
            struct _stat sourceStat;
            if (_stat(commandName, &sourceStat) == 0 &&
                difftime(sourceStat.st_mtime, matchedEntry->fileTime) > 0.0) {
                usePreparedStream = 0;
            }
            if (usePreparedStream != 0) {
                FILE *const stream = preparedIndexStream;
                if (fseek(stream, matchedEntry->fileOffset, SEEK_SET) == 0) {
                    return stream;
                }
            }
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-validateargsandnodetype
 * @recoil-artifact defines .text recoil:function:0x4c5820: zInterp_Context::ValidateArgsAndNodeType.
 *
 * Purpose: validate argument count and optional zClass node type for commands.
 */
bool zInterp_Context::ValidateArgsAndNodeType(
    int expectedArgCount,
    int expectedClassType,
    zClass_NodePartial *node
) {
    if (expectedClassType != 0) {
        if (node == 0) {
            char *commandToken = tokenCount > 0 ? tokenList[0] : 0;
            ReportErrorf(
                this,
                "Interp: keyword [%s] has NULL node to work with",
                commandToken
            );
            return 0;
        }

        const int classType = node->classId;
        if (classType != expectedClassType) {
            ReportErrorf(
                this,
                "Interp: keyword [%s] has node [%s] of class=%d, expected=%d",
                tokenCount > 0 ? tokenList[0] : 0,
                node,
                classType,
                expectedClassType
            );
            return 0;
        }
    }

    const unsigned int currentTokenCount = tokenCount;
    if ((unsigned int)(expectedArgCount + 1) > (unsigned int)(currentTokenCount)) {
        ReportErrorf(
            this,
            "Interp: keyword [%s] requires (%d) args, found (%d) args",
            currentTokenCount > 0 ? tokenList[0] : 0,
            expectedArgCount,
            currentTokenCount - 1
        );
        return false;
    }
    return true;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-defaultdispatchhook
 * @recoil-artifact defines .text recoil:function:0x4c58c0: zInterp_Context::DefaultDispatchHook.
 *
 * Both retail scroll-disable callers supply the context in ECX and the node
 * on the stack. There are no callback-storage references to this body.
 * Purpose: process a scroll-disable request by touching the node user-data
 * provider entry and returning false.
 */
bool zInterp_Context::DefaultDispatchHook(
    zClass_NodePartial *node
) {
    if (node != 0) {
        zClass_Class::gwNodeGetUserData(
            node,
            0
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-context-registerscrollalwaysnode
 * @recoil-artifact defines .text recoil:function:0x4c58e0: zInterp_Context::RegisterScrollAlwaysNode.
 *
 * Purpose: register a node for immediate or driver-driven texture scrolling.
 */
bool zInterp_Context::RegisterScrollAlwaysNode(
    zClass_NodePartial *node,
    float scrollRateU,
    float scrollRateV,
    bool installDriverCallback
) {
    if (node != 0) {
        unsigned int diValue = 0;
        zClass_Class::gwNodeGetUserData(node, &diValue);
        zDiPartial *const di = (zDiPartial *)(diValue);
        if (di != 0) {
            zModel::SetDiTextureWorldPerMeter(
                di, 1, scrollRateU, scrollRateV
            );
            if (installDriverCallback != 0) {
                if (scrollAlwaysDriverNode == 0) {
                    scrollAlwaysDriverNode = zClass_Object3D::gwObject3DInit();
                    zClass_Class::gwNodeSetActionCallback(
                        scrollAlwaysDriverNode,
                        (void *)(&zInterp_Object3D::ScrollAlwaysTickAction)
                    );
                    zClass_Class::gwNodeSetName(
                        scrollAlwaysDriverNode,
                        g_zInterp_ScrollAlwaysNodeName
                    );
                    scrollAlwaysDriverNode->callbackContext =
                        (zClass_NodePartial *)(this);
                }

                scrollAlwaysList.push_back(node);
            } else {
                zClass_Class::gwNodeSetActionCallback(
                    node,
                    (void *)(&zInterp_Object3D::DefaultRenderAction)
                );
            }
            return true;
        }
    }
    return false;
}

namespace zInterp_Object3D {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-object3d-defaultrenderaction
 * @recoil-artifact defines .text recoil:function:0x4c59e0: zInterp_Object3D::DefaultRenderAction.
 *
 * Purpose: update scrolling textures for a node's display-instance payload.
 */
int __fastcall DefaultRenderAction(
    zClass_NodePartial *node
) {
    unsigned int userData;
    zClass_Class::gwNodeGetUserData(
        node,
        &userData
    );
    return zModel_Instance_UpdateScrollingTexturesIfNeeded((zModel_InstancePartial *)(userData));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinterp-zinterp-parse-zinterp-object3d-scrollalwaystickaction
 * @recoil-artifact defines .text recoil:function:0x4c5a00: zInterp_Object3D::ScrollAlwaysTickAction.
 *
 * Purpose: walk the context-owned always-scroll list and run the texture
 * update action for each payload node.
 */
void __fastcall ScrollAlwaysTickAction(
    zClass_NodePartial *wrapperNode
) {
    if (wrapperNode == 0) {
        return;
    }

    zInterp_Context *const context = (zInterp_Context *)(wrapperNode->callbackContext);
    zInterp_ScrollAlwaysList::iterator entry = context->scrollAlwaysList.begin();
    while (entry != context->scrollAlwaysList.end()) {
        DefaultRenderAction(*entry);
        ++entry;
    }
}

} // namespace zInterp_Object3D
