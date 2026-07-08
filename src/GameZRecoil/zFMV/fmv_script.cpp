#include "Battlesport/recoil_app.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <mmsystem.h>
#include <digitalv.h>
#include <vfw.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

extern "C" HWND g_RecoilApp_hWndMain;

struct zFMV_ActionTagStringSet {
    char playSoundTag[10];
    char padAfterPlaySound[2];
    char blurVTag[6];
    char padAfterBlurV[2];
    char blurHTag[6];
    char padAfterBlurH[2];
    char blurTag[5];
    char padAfterBlur[3];
    char playMciTag[8];
    char playAviTag[8];
    char fadeOutTag[8];
    char fadeInTag[7];
    char padAfterFadeIn[1];
    char waitTag[5];
    char padAfterWait[3];
    char loadImageTag[10];
    char padAfterLoadImage[2];
    char blitImageTag[10];
    char padAfterBlitImage[2];
    char showImageTag[10];
    char padAfterShowImage[2];
};

extern "C" {
/**
 * Reimplements data 0x4dfb1c: g_zFMV_MpegVideoString.
 * BN xrefs: zFMV_Playback::OpenAndPlay opens the MCI MPEGVideo device type.
 * Purpose: first literal in the playback/MCI data owner 0x4dfb1c..0x4dfb63.
 */
char g_zFMV_MpegVideoString[] = "MPEGVideo";

/**
 * Reimplements data 0x4dfb28: g_zFMV_SourceFile_FmvMainCpp.
 * BN xrefs: zFMV_Playback::ReportMciError passes the retail source path to zError.
 * Purpose: second literal in the playback/MCI data owner 0x4dfb1c..0x4dfb63.
 */
char g_zFMV_SourceFile_FmvMainCpp[] = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_main.cpp";

/**
 * Reimplements data 0x4dfb50: g_zFMV_UnknownErrorIdMsg.
 * BN xrefs: zFMV_Playback::ReportMciError uses this fallback MCI error text.
 * Purpose: final literal in the playback/MCI data owner 0x4dfb1c..0x4dfb63.
 */
char g_zFMV_UnknownErrorIdMsg[] = "Unknown Error ID";

/**
 * Reimplements data 0x4dfb64: g_zFMV_ParseActionsErrorFmt.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd reports malformed action arrays.
 * Purpose: FMV script action parse diagnostic format in retail .data order;
 * excludes the adjacent action-tag owner beginning at 0x4dfb94.
 */
char g_zFMV_ParseActionsErrorFmt[] = "Error in parsing fmv actions:  file=%s, tag=%s";

/**
 * Reimplements data owner 0x4dfb94..0x4dfc03:
 * g_zFMV_ActionPlaySoundTag, g_zFMV_ActionBlurVTag,
 * g_zFMV_ActionBlurHTag, g_zFMV_ActionBlurTag,
 * g_zFMV_ActionPlayMciTag, g_zFMV_ActionPlayAviTag,
 * g_zFMV_ActionFadeOutTag, g_zFMV_ActionFadeInTag,
 * g_zFMV_ActionWaitTag, g_zFMV_ActionLoadImageTag,
 * g_zFMV_ActionBlitImageTag, and g_zFMV_ActionShowImageTag.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd compares action node tags.
 * Purpose: packed FMV action selector strings in retail .data order; excludes
 * the adjacent IMAGE_PATH key at 0x4dfc04.
 */
zFMV_ActionTagStringSet g_zFMV_ActionTagStrings = {
    "PLAYSOUND",
    {0, 0},
    "BLURV",
    {0, 0},
    "BLURH",
    {0, 0},
    "BLUR",
    {0, 0, 0},
    "PLAYMCI",
    "PLAYAVI",
    "FADEOUT",
    "FADEIN",
    {0},
    "WAIT",
    {0, 0, 0},
    "LOADIMAGE",
    {0, 0},
    "BLITIMAGE",
    {0, 0},
    "SHOWIMAGE",
    {0, 0}
};

/**
 * Reimplements data 0x4dfc04: zHudCfgKey_IMAGE_PATH.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd reads the image media root path.
 * Purpose: named FMV script image key literal in retail .data order.
 */
char zHudCfgKey_IMAGE_PATH[] = "IMAGE_PATH";

/**
 * Reimplements data 0x4dfc10: g_zFMV_PathKey.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd reads the FMV media root path.
 * Purpose: named FMV script key literal in retail .data order.
 */
char g_zFMV_PathKey[] = "FMV_PATH";

/**
 * Reimplements data 0x4dfc1c: g_zFMV_SourceFile_FmvScriptCpp.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd passes the retail source path to zError.
 * Purpose: source-file literal for FMV script diagnostics.
 */
char g_zFMV_SourceFile_FmvScriptCpp[] = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_script.cpp";

/**
 * Reimplements data 0x4dfc44: g_zFMV_MissingDefinitionsZrdErrorMsg.
 * BN xrefs: zFMV_Script::LoadActionsFromZrd reports missing FMV definitions.
 * Purpose: missing fmv.zrd diagnostic string.
 */
char g_zFMV_MissingDefinitionsZrdErrorMsg[] = "Failed to find FMV definitions (fmv.zrd)";

/**
 * Reimplements data 0x4dfc70: g_zSnd_FmvSampleSetName.
 * BN xrefs: zFMV_Script::BeginCurrentAction initializes the FMV sample set by name.
 * Purpose: zSnd sample-set selector literal adjacent to the FMV stream diagnostics.
 */
char g_zSnd_FmvSampleSetName[] = "FMV";

/**
 * Reimplements data 0x4dfc74: g_zFMV_SourceFile_FmvStreamCpp.
 * BN xrefs: zFMV_Stream error paths pass the retail source path to zError.
 * Purpose: source-file literal for AVI stream diagnostics.
 */
char g_zFMV_SourceFile_FmvStreamCpp[] = "D:\\Proj\\GameZRecoil\\zFMV\\fmv_stream.cpp";

/**
 * Reimplements data 0x4dfc9c: g_zFMV_CannotReadAviStreamInfoMsg.
 * BN xrefs: zFMV_Stream::Constructor reports AVI video stream-info failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviStreamInfoMsg[] = "Cannot Read AVI Stream Info";

/**
 * Reimplements data 0x4dfcb8: g_zFMV_CannotReadAviFormatMsg.
 * BN xrefs: zFMV_Stream::Constructor reports AVI video format read failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviFormatMsg[] = "Cannot Read AVI Format";

/**
 * Reimplements data 0x4dfcd0: g_zFMV_CannotReadAviFormatSizeMsg.
 * BN xrefs: zFMV_Stream::Constructor reports AVI video format-size failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviFormatSizeMsg[] = "Cannot Read AVI Format Size";

/**
 * Reimplements data 0x4dfcec: g_zFMV_CannotOpenAviFileMsg.
 * BN xrefs: zFMV_Stream::Constructor reports AVI video stream open failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotOpenAviFileMsg[] = "Cannot Open AVI File";

/**
 * Reimplements data 0x4dfd04: g_zFMV_CannotReadAviSoundStreamMsg.
 * BN xrefs: zFMV_Stream audio read paths report AVI sound stream failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviSoundStreamMsg[] = "Cannot Read AVI Sound Stream";

/**
 * Reimplements data 0x4dfd24: g_zFMV_CannotReadAviSoundStreamInfoMsg.
 * BN xrefs: zFMV_Stream::OpenAudio reports AVI sound stream-info failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviSoundStreamInfoMsg[] = "Cannot Read AVI Sound Stream Info";

/**
 * Reimplements data 0x4dfd48: g_zFMV_CannotReadAviSoundFormatMsg.
 * BN xrefs: zFMV_Stream::OpenAudio reports AVI sound format read failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviSoundFormatMsg[] = "Cannot Read AVI Sound Format";

/**
 * Reimplements data 0x4dfd68: g_zFMV_CannotReadAviSoundFormatSizeMsg.
 * BN xrefs: zFMV_Stream::OpenAudio reports AVI sound format-size failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviSoundFormatSizeMsg[] = "Cannot Read AVI Sound Format Size";

/**
 * Reimplements data 0x4dfd8c: g_zFMV_CannotDecompressAviVideoStreamMsg.
 * BN xrefs: zFMV_Stream::ReadAndDecodeFrame reports video decompression failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotDecompressAviVideoStreamMsg[] = "Cannot Decompress AVI Video Stream";

/**
 * Reimplements data 0x4dfdb0: g_zFMV_CannotReadAviVideoStreamMsg.
 * BN xrefs: zFMV_Stream::ReadAndDecodeFrame reports AVI video read failures.
 * Purpose: AVI stream diagnostic literal in retail .data order.
 */
char g_zFMV_CannotReadAviVideoStreamMsg[] = "Cannot Read AVI Video Stream";
}

/**
 * Reimplements data 0x53a708-0x53a714: g_zFMV_ActionImage_ActiveRegion.
 * BN xrefs: zFMV_ActionImage::ConstructorScaled clears the origin, writes
 * width and height through zRndr::GetActiveRegionState, then copies the full
 * record into the action instance.
 * Purpose: transient active-region rectangle used while constructing scaled
 * image actions.
 */
extern "C" zVidRect32 g_zFMV_ActionImage_ActiveRegion = {0};

/**
 * Reimplements data 0x53a718-0x53a727: g_zFMV_ActionPlayMci_DestRect.
 * BN xrefs: zFMV_ActionPlayMci::Constructor clears the origin, writes width
 * and height through zRndr::GetActiveRegionState, then passes the complete
 * rect to zFMV_Playback::SetDestRect.
 * Purpose: transient MCI playback destination rectangle, semantically separate
 * from the adjacent ActionImage rectangle globals in retail .data order.
 */
extern "C" zFMV_Rect g_zFMV_ActionPlayMci_DestRect = {0};

/**
 * Reimplements data 0x53a728-0x53a734: g_zFMV_ActionImage_BlitRect.
 * BN xrefs: zFMV_ActionImage::ConstructorWithScreenRect writes the origin and
 * copies the full record into the action instance.
 * Purpose: transient screen-origin rectangle used while constructing image
 * actions.
 */
extern "C" zVidRect32 g_zFMV_ActionImage_BlitRect = {0};
// BN 0x4d2580 is a single float consumed by the multimedia-timer wrappers.
extern "C" const float g_zFMV_ScriptTimeGetTimeToSecondsScale = 0.00100000005f;

namespace {
const int k_zFMV_RendererBackendSoftware = 0;
const int k_zFMV_RendererBackend3dfx = 2;
const int k_zFMV_BlurModeHorizontal = 1;
const int k_zFMV_BlurModeVertical = 2;
const int k_zFMV_BlurModeCombined = 3;

struct zFMV_MciWindowParams {
    DWORD_PTR callback;
    HWND hwnd;
    unsigned int commandShow;
    const char *text;
};

struct zFMV_MciRectParams {
    DWORD_PTR callback;
    int left;
    int top;
    int width;
    int height;
};

struct zFMV_MciSetParams {
    DWORD_PTR callback;
    DWORD timeFormat;
    DWORD audio;
};

struct zFMV_MciPlayParams {
    DWORD_PTR callback;
    DWORD from;
    DWORD to;
};

/**
 * Original inline helper evidence: BN 0x4631f0 copies the active-region rect
 * state through a destination rect pointer after zRndr::GetActiveRegionState.
 * Purpose: transfer the recovered active render region into an FMV blit rect.
 */
static inline void CopyActionImageActiveRegionRect(
    zVidRect32 *rect
) {
    *rect = g_zFMV_ActionImage_ActiveRegion;
}

/**
 * Observed in callers 0x462330, 0x4631af, 0x463221, 0x4635af, and 0x463b2f.
 * Original inline helper evidence: recovered from address-backed callers in this source file.
 * Purpose: duplicate an input C string through the active C runtime spelling.
 */
static inline char *DuplicateCString(
    const char *value
) {
#if defined(_MSC_VER)
    return _strdup(value);
#else
    return strdup(value);
#endif
}

/**
 * Observed in caller 0x4626b0.
 * Original inline helper evidence: recovered from address-backed callers in this source file.
 * Purpose: return the first node of a zReader array payload.
 */
zReader::Node *ArrayBase(
    zReader::Node *node
) {
    return node->value.nodes;
}

/**
 * Observed in caller 0x4626b0.
 * Original inline helper evidence: recovered from address-backed callers in this source file.
 * Purpose: return one indexed zReader array element.
 */
zReader::Node *ArrayItem(
    zReader::Node *node,
    int index
) {
    return &ArrayBase(node)[index];
}

/**
 * Observed in caller 0x4626b0.
 * Original inline helper evidence: recovered from address-backed callers in this source file.
 * Purpose: fetch a string argument from an FMV action node.
 */
const char *StringArg(
    zReader::Node *actionNode,
    int index
) {
    zReader::Node *arg = ArrayItem(
        actionNode,
        index
    );
    return arg->type == zReader::ZRDR_NODE_STRING ? arg->value.str : 0;
}

} // namespace

/* Source-file block layout: the current native build still compiles this compatibility container.
 * The included fragment files below hold the ledger physical source rows.
 */
#include "fmv_main.cpp"
/**
 * Reimplements 0x4625e0: zFMV_Script::Init.
 * Purpose: initialize an FMV script object and optionally load its action sequence.
 */
zFMV_Script * zFMV_Script::Init(
    const char *zrdPath,
    const char *tagPrefix,
    HWND hWnd
) {
    m_hWnd = hWnd != 0 ? hWnd : g_RecoilApp_hWndMain;
    m_fmvPath = 0;
    m_head = 0;
    m_tail = 0;
    m_cur = 0;
    m_abortOnKey = 1;

    if (zrdPath != 0 && tagPrefix != 0) {
        LoadActionsFromZrd(
            zrdPath,
            tagPrefix
        );
    }

    return this;
}

/**
 * Reimplements 0x462630: zFMV_Script::Cleanup.
 * Purpose: free the FMV path and destroy all loaded script actions.
 */
void zFMV_Script::Cleanup() {
    if (m_fmvPath != 0) {
        free(m_fmvPath);
        m_fmvPath = 0;
    }

    Reset(1);
}

/**
 * Reimplements 0x462660: zFMV_Script::Reset.
 * Purpose: reset the current action pointer and optionally destroy the loaded action list.
 */
void zFMV_Script::Reset(
    int destroyActions
) {
    zFMV_Action *action = m_head;
    if (destroyActions != 0) {
        while (action != 0) {
            zFMV_Action *const next = action->next;
            if (action != 0) {
                delete action;
            }
            action = next;
        }

        m_tail = 0;
        m_cur = 0;
        m_head = 0;
        return;
    }

    {
        m_cur = action;
        return;
    }
}

/**
 * Reimplements 0x4626b0: zFMV_Script::LoadActionsFromZrd.
 * Purpose: load FMV path metadata and construct actions from a named zReader sequence.
 */
int zFMV_Script::LoadActionsFromZrd(
    const char *zrdPath,
    const char *tagPrefix
) {
    zReader::Node *root = zReader::LoadNodeFromPath(
        zrdPath,
        0,
        0
    );
    if (root == 0) {
        zError::ReportOld(
            0x200,
            g_zFMV_SourceFile_FmvScriptCpp,
            0x51,
            g_zFMV_MissingDefinitionsZrdErrorMsg
        );
        return -1;
    }

    m_fmvPath = _strdup(zReader::ReadNamedString(
        root,
        g_zFMV_PathKey
    ));
    zImage_InitMissionResources(zReader::ReadNamedString(
        root,
        zHudCfgKey_IMAGE_PATH
    ));

    zReader::Node *sequenceNode = zReader_GetNamedNode(
        root,
        tagPrefix
    );
    if (sequenceNode == 0) {
        return 0;
    }

    int i = 0;
    int result = sequenceNode->value.nodes[0].value.i32 - 1;
    int actionIndex = 1;
    for (; i < result; ++i, ++actionIndex) {
        zReader::Node *actionNode = &sequenceNode->value.nodes[actionIndex];
        if (actionNode->type != zReader::ZRDR_NODE_ARRAY) {
            result = 0;
            zError::ReportOld(
                0x200,
                g_zFMV_SourceFile_FmvScriptCpp,
                0x69,
                g_zFMV_ParseActionsErrorFmt,
                zrdPath,
                tagPrefix
            );
            break;
        }

        const char *actionTag = actionNode->value.nodes[1].value.str;

        if (strcmp(
                actionTag,
                g_zFMV_ActionTagStrings.showImageTag
            ) == 0) {
            AppendAction(new zFMV_ActionImage(
                actionNode->value.nodes[2].value.str,
                1
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.blitImageTag
                   ) == 0) {
            AppendAction(new zFMV_ActionImage(
                actionNode->value.nodes[2].value.str,
                1,
                actionNode->value.nodes[3].value.i32,
                actionNode->value.nodes[4].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.loadImageTag
                   ) == 0) {
            AppendAction(new zFMV_ActionImage(
                actionNode->value.nodes[2].value.str,
                0
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.waitTag
                   ) == 0) {
            AppendAction(new zFMV_ActionWait(actionNode->value.nodes[2].value.f32));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.fadeInTag
                   ) == 0) {
            AppendAction(new zFMV_ActionFade(
                actionNode->value.nodes[2].value.nodes[1].value.i32,
                actionNode->value.nodes[2].value.nodes[2].value.i32,
                actionNode->value.nodes[2].value.nodes[3].value.i32,
                actionNode->value.nodes[3].value.u32,
                -1,
                actionNode->value.nodes[4].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.fadeOutTag
                   ) == 0) {
            AppendAction(new zFMV_ActionFade(
                actionNode->value.nodes[2].value.nodes[1].value.i32,
                actionNode->value.nodes[2].value.nodes[2].value.i32,
                actionNode->value.nodes[2].value.nodes[3].value.i32,
                actionNode->value.nodes[3].value.u32,
                1,
                actionNode->value.nodes[4].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.playAviTag
                   ) == 0) {
            const int actionArgCount = actionNode->value.nodes[0].value.i32;
            if (actionArgCount > 3) {
                AppendAction(new zFMV_ActionPlayAvi(
                    m_fmvPath,
                    actionNode->value.nodes[2].value.str,
                    actionNode->value.nodes[3].value.i32
                ));
            } else {
                AppendAction(new zFMV_ActionPlayAvi(
                    m_fmvPath,
                    actionNode->value.nodes[2].value.str,
                    0
                ));
            }
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.playMciTag
                   ) == 0) {
            AppendAction(new zFMV_ActionPlayMci(
                m_hWnd,
                m_fmvPath,
                actionNode->value.nodes[2].value.str
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.blurTag
                   ) == 0) {
            AppendAction(new zFMV_ActionBlur(
                1,
                actionNode->value.nodes[2].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.blurHTag
                   ) == 0) {
            AppendAction(new zFMV_ActionBlurH(
                1,
                actionNode->value.nodes[2].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.blurVTag
                   ) == 0) {
            AppendAction(new zFMV_ActionBlurV(
                1,
                actionNode->value.nodes[2].value.i32
            ));
        } else if (strcmp(
                       actionTag,
                       g_zFMV_ActionTagStrings.playSoundTag
            ) == 0) {
            AppendAction(new zFMV_ActionPlaySound(actionNode->value.nodes[2].value.str));
        }
    }

    zReader::FreeLoadedTree(root);
    return result;
}

/**
 * Reimplements 0x462e30: zFMV_Action::RunBlockingImmediate.
 * Purpose: run an action to completion without advancing elapsed time.
 */
void zFMV_Action::RunBlockingImmediate() {
    Begin(0.0);
    while (Update(0.0) != 0) {
    }
    End();
}

/**
 * Reimplements 0x462e90: zFMV_ActionPlaySound::Begin.
 * Purpose: find and play the named FMV sound sample.
 */
void zFMV_ActionPlaySound::Begin(
    double
) {
    sample = zSnd::FindSampleByName(sampleName);
    if (voice != 0) {
        voice->StopIfActive();
    }
    if (sample != 0) {
        voice = sample->PlayA3DSimple(1.0f);
    }
}

/**
 * Reimplements 0x462ed0: zFMV_ActionWait::Begin.
 * Purpose: capture the wait action start time.
 */
void zFMV_ActionWait::Begin(
    double timeSec
) {
    startSec = (float)(timeSec);
}

/**
 * Reimplements 0x462ee0: zFMV_ActionWait::Update.
 * Purpose: keep the wait action active until its duration has elapsed.
 */
int zFMV_ActionWait::Update(
    double timeSec
) {
    return timeSec < (double)(startSec + durationSec) ? 1 : 0;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_ActionWait virtual slot contract.
 * Purpose: restore FMV surfaces when a wait action completes.
 */
void zFMV_ActionWait::End() {
    FlipSurfaces();
}

/**
 * Reimplements 0x462f00: zFMV_Action::FlipSurfaces.
 * Purpose: restore adjusted video surfaces after an FMV action completes.
 */
void zFMV_Action::FlipSurfaces() {
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
}

/**
 * Reimplements 0x462f10: zFMV_Script::AppendAction.
 * Purpose: append an action to the script's singly linked action list.
 */
int zFMV_Script::AppendAction(
    zFMV_Action *action
) {
    if (action == 0) {
        return 0;
    }

    action->next = 0;
    if (m_tail == 0) {
        *(zFMV_Action *volatile *)(&m_tail) = action;
        *(zFMV_Action *volatile *)(&m_head) = action;
        *(zFMV_Action *volatile *)(&m_cur) = action;
        return 1;
    }

    m_tail->next = action;
    m_tail = action;
    return 1;
}

/**
 * Reimplements 0x462f50: zFMV_Script::RunBlocking.
 * Purpose: run the loaded action sequence synchronously until completion.
 */
int zFMV_Script::RunBlocking(
    int abortOnKey
) {
    m_abortOnKey = abortOnKey;
    BeginAtTime();
    if (UpdateAtTime() != 0) {
        do {
        } while (UpdateAtTime() != 0);
    }

    BeginNow(0);
    return 1;
}

/**
 * Reimplements 0x462f90: zFMV_Script::BeginCurrentAction.
 * Purpose: prepare render/input/sound state and begin the current action.
 */
int zFMV_Script::BeginCurrentAction(
    double startTimeSec
) {
    if (m_cur == 0) {
        return 0;
    }

    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    zSndSampleSet_InitByName(g_zSnd_FmvSampleSetName);
    zInput::Keyboard_ResetTransitionState();
    m_startTimeSec = startTimeSec;
    m_cur->Begin(0.0);
    return 1;
}

/**
 * Reimplements 0x463000: zFMV_Script::Update.
 * Purpose: advance the current action, handle abort input, and start the next action.
 */
int zFMV_Script::Update(
    double timeSec
) {
    if (m_cur == 0) {
        return 0;
    }

    if (m_abortOnKey != 0) {
        if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
            m_cur->End();
            zSndPlayHandleSnapshot::CreateFromActiveSamples()->StopAllIfPlaying();
            m_cur = 0;
            return 0;
        }

        zInput::PollActiveDevices(1);
    }

    const double relativeTimeSec = timeSec - m_startTimeSec;
    if (m_cur->Update(relativeTimeSec) == 0) {
        m_cur->End();
        zFMV_Action *const next = m_cur->next;
        m_cur = next;
        if (next != 0) {
            next->Begin(relativeTimeSec);
        }
    }

    return 1;
}

/**
 * Reimplements 0x4630a0: zFMV_Script::BeginAtTime.
 * Purpose: begin the current action using the current multimedia timer time.
 */
int zFMV_Script::BeginAtTime() {
    return BeginCurrentAction((double)(timeGetTime()) * g_zFMV_ScriptTimeGetTimeToSecondsScale);
}

/**
 * Reimplements 0x4630e0: zFMV_Script::UpdateAtTime.
 * Purpose: update the script using the current multimedia timer time.
 */
int zFMV_Script::UpdateAtTime() {
    return Update((double)(timeGetTime()) * g_zFMV_ScriptTimeGetTimeToSecondsScale);
}

/**
 * Reimplements 0x463120: zFMV_Script::BeginNow.
 * Purpose: reset the script action cursor, optionally destroying loaded actions.
 */
void zFMV_Script::BeginNow(
    int destroyActions
) {
    Reset(destroyActions);
}

/**
 * Reimplements 0x463130: zFMV_ActionImage::ConstructorWithScreenRect.
 * Purpose: initialize an image action with an explicit screen blit origin.
 */
zFMV_ActionImage::zFMV_ActionImage(
    const char *path,
    int adjustSurfaces,
    int blitX,
    int blitY
) {
    image = 0;
#if defined(_MSC_VER)
    imagePath = _strdup(path);
#else
    imagePath = strdup(path);
#endif
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_BlitRect.top = blitY;
    g_zFMV_ActionImage_BlitRect.left = blitX;
    forcePrimaryPostprocess = 1;
    blitRect = g_zFMV_ActionImage_BlitRect;
}

/**
 * Reimplements 0x4631f0: zFMV_ActionImage::ConstructorScaled.
 * Purpose: initialize an image action sized to the active render region.
 */
zFMV_ActionImage::zFMV_ActionImage(
    const char *path,
    int adjustSurfaces
) {
    image = 0;
#if defined(_MSC_VER)
    imagePath = _strdup(path);
#else
    imagePath = strdup(path);
#endif
    doAdjustSurfaces = adjustSurfaces;
    g_zFMV_ActionImage_ActiveRegion.top = 0;
    g_zFMV_ActionImage_ActiveRegion.left = 0;
    forcePrimaryPostprocess = 0;

    int discard;
    zRndr::GetActiveRegionState(
        &g_zFMV_ActionImage_ActiveRegion.right,
        &g_zFMV_ActionImage_ActiveRegion.bottom,
        &discard,
        &discard
    );

    CopyActionImageActiveRegionRect(&blitRect);
}

/**
 * Reimplements 0x4632a0: zFMV_ActionImage::~zFMV_ActionImage.
 * Purpose: end image playback and free the image path.
 */
zFMV_ActionImage::~zFMV_ActionImage() {
    End();
    if (imagePath != 0) {
        free(imagePath);
        imagePath = 0;
    }
}

/**
 * Reimplements 0x463300: zFMV_ActionImage::Begin.
 * Purpose: resolve the image resource used by this FMV image action.
 */
void zFMV_ActionImage::Begin(double) {
    image = zImage::TexDir_FindOrCreateByPath(imagePath);
}

/**
 * Reimplements 0x463320: zFMV_ActionImage::Update.
 * Purpose: blit the resolved image through the active renderer path and finish immediately.
 */
int zFMV_ActionImage::Update(double) {
    int iterations =
        g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware ? 2 : 1;

    if (image != 0) {
        do {
            if (forcePrimaryPostprocess != 0 ||
                g_zVideo_ActiveRendererPath == k_zFMV_RendererBackend3dfx) {
                zVideo::RunPostprocessOnPrimaryBuffer();
                zVid_Image::BlitToActiveTarget(
                    (zVidImagePartial *)(image),
                    blitRect.left,
                    blitRect.top,
                    0,
                    0
                );
                zVideo::Dispatch_UnlockPrimarySurfaceState();
            } else {
                g_zVideo_pfnBltSwToPrimaryRect(
                    (zVidImagePartial *)(image),
                    0,
                    0,
                    &blitRect
                );
            }

            if (doAdjustSurfaces != 0) {
                zVideo::AdjustSurfacesIfEnabled(
                    0,
                    0,
                    1,
                    1
                );
            }
            --iterations;
        } while (iterations != 0);
    }

    return 0;
}

/**
 * Reimplements 0x4633a0: zFMV_ActionImage::End.
 * Purpose: release the resolved image resource.
 */
void zFMV_ActionImage::End() {
    if (image != 0) {
        zVid_Image::ReleaseIfNotDefault((zVidImagePartial *)(image));
        image = 0;
    }
}

/**
 * Reimplements 0x4633c0: zFMV_ActionFade::Constructor.
 * Purpose: initialize fade color, duration, direction, and alpha settings.
 */
zFMV_ActionFade::zFMV_ActionFade(
    int red,
    int green,
    int blue,
    unsigned int duration,
    int direction,
    int alpha
) {
    fadeColorPacked16 = (unsigned short)(zVid_PackColorRGB(
        red,
        green,
        blue
    ));
    durationSecRaw = duration;
    fadeDirectionSign = direction;
    maxAlpha = alpha;
}

/**
 * Reimplements 0x463410: zFMV_ActionFade::Begin.
 * Purpose: capture the current surface and record the fade start time.
 *
 * Data audit: Begin/End have no direct authored global data references.
 * Update reads only the accepted zVideo renderer-dispatch globals
 * g_zVideo_ActiveRendererPath and g_zVideo_pfnFlushQuadBatch.
 */
void zFMV_ActionFade::Begin(double timeSec) {
    capturedFrame = zVideo_buff_CaptureSurfaceToImage(1);
    startSec = timeSec;
}

/**
 * Reimplements 0x463440: zFMV_ActionFade::Update.
 * Purpose: composite the captured frame with a timed fade overlay.
 */
int zFMV_ActionFade::Update(double timeSec) {
    if (capturedFrame == 0) {
        return 0;
    }

    double fadeProgress = (timeSec - startSec) / *(float *)&durationSecRaw;
    int result = 1;
    if (fadeDirectionSign < 0) {
        fadeProgress = 1.0 - fadeProgress;
        if (fadeProgress <= 0.0) {
            fadeProgress = 0.0;
            result = 0;
        }
    } else {
        if (fadeProgress > 1.0) {
            fadeProgress = 1.0;
            result = 0;
        }
    }

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnSwBuffer();
    } else {
        zVideo::RunPostprocessOnPrimaryBuffer();
    }

    zVid_Image::BlitToActiveTarget(
        (zVidImagePartial *)(capturedFrame),
        0,
        0,
        0,
        0
    );

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::Dispatch_UnlockSwSurfaceState();
    }

    zRndr_OverlayRect_Submit(
        fadeColorPacked16,
        0,
        (double)(maxAlpha) * fadeProgress
    );

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideoD3D::SceneEnter();
        g_zVideo_pfnFlushQuadBatch();
        zVideoD3D::SceneLeave();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            0
        );
    } else {
        zRndr_OverlayRect_FlushSw();
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            1,
            1
        );
    }

    return result;
}

/**
 * Reimplements 0x463550: zFMV_ActionFade::End.
 * Purpose: release the captured fade frame.
 */
void zFMV_ActionFade::End() {
    if (capturedFrame != 0) {
        zVid_Image::ReleaseIfNotDefault((zVidImagePartial *)(capturedFrame));
        capturedFrame = 0;
    }
}

/**
 * Reimplements 0x463570: zFMV_ActionPlayAvi::Constructor.
 * Purpose: build the AVI media path, resolve CD-ROM fallback, and store mode flags.
 */
zFMV_ActionPlayAvi::zFMV_ActionPlayAvi(
    const char *mediaRootPath,
    const char *mediaFileName,
    int flags
) {
    mediaPath = (char *)(calloc(
        strlen(mediaRootPath) + strlen(mediaFileName) + 0x1b,
        1
    ));
    sprintf(
        mediaPath,
        "%s\\%s",
        mediaRootPath,
        mediaFileName
    );
    modeFlags = flags;

    struct stat statBuffer;
    if (stat(
        mediaPath,
        &statBuffer
    ) == -1) {
        char *resolvedPath = zSys::FindFileOnDriveType(
            DRIVE_CDROM,
            mediaPath,
            0
        );
        if (resolvedPath != 0) {
            strcpy(
                mediaPath,
                resolvedPath
            );
        }
    }
}

/**
 * Reimplements 0x463670: zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi.
 * Purpose: free the AVI media path.
 */
zFMV_ActionPlayAvi::~zFMV_ActionPlayAvi() {
    if (mediaPath != 0) {
        free(mediaPath);
        mediaPath = 0;
    }
}

/**
 * Reimplements 0x4636d0: zFMV_ActionPlayAvi::Update.
 * Purpose: advance AVI frame playback, blit the decoded frame, and update surfaces.
 */
int zFMV_ActionPlayAvi::Update(
    double timeSec
) {
    int result = 1;
    const int previousFrameIndex = lastDecodedFrameIndex;
    if (previousFrameIndex < 0) {
        startTimeSec = timeSec;
    }

    zFMV_Stream *const playbackStream = stream;
    const int frameIndex =
        (int)((timeSec - startTimeSec) * (double)(playbackStream->videoFramesPerSecond));
    if (frameIndex != previousFrameIndex) {
        int blitPrimaryToSwFirst = 0;
        if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackend3dfx) {
            zVideo::RunPostprocessOnPrimaryBuffer();
            result = playbackStream->ReadAndDecodeFrame(frameIndex);
            zVideo::Dispatch_UnlockPrimarySurfaceState();
            g_zVideo_pfnBltSwToPrimaryRect(
                (zVidImagePartial *)(stream),
                0,
                0,
                (zVidRect32 *)(&destRect)
            );
            blitPrimaryToSwFirst = 1;
        } else {
            result = playbackStream->ReadAndDecodeFrame(frameIndex);
            g_zVideo_pfnBltSwToPrimaryRect(
                (zVidImagePartial *)(stream),
                0,
                0,
                (zVidRect32 *)(&destRect)
            );
        }

        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            1,
            blitPrimaryToSwFirst
        );
        lastDecodedFrameIndex = frameIndex;
    }

    return result;
}

/**
 * Reimplements 0x463790: zFMV_ActionPlayAvi::Begin.
 * Purpose: allocate and initialize the AVI stream and active destination rectangle.
 */
void zFMV_ActionPlayAvi::Begin(
    double
) {
    zFMV_Stream *const streamStorage = (zFMV_Stream *)(::operator new(sizeof(zFMV_Stream)));
    zFMV_Stream *initializedStream = 0;
    if (streamStorage != 0) {
        initializedStream = streamStorage->Init(
            mediaPath,
            modeFlags
        );
    }
    stream = initializedStream;

    destRect.top = 0;
    destRect.left = 0;
    int discard = 0;
    zRndr::GetActiveRegionState(
        &destRect.right,
        &destRect.bottom,
        &discard,
        &discard
    );
    lastDecodedFrameIndex = -1;
}

/**
 * Reimplements 0x463820: zFMV_ActionPlayAvi::End.
 * Purpose: destroy the AVI stream object and clear the stream pointer.
 */
void zFMV_ActionPlayAvi::End() {
    zFMV_Stream *const playbackStream = stream;
    if (playbackStream != 0) {
        playbackStream->Destructor();
        ::operator delete(playbackStream);
    }
    stream = 0;
}

/**
 * Reimplements 0x463850: zFMV_ActionBlur::Constructor.
 * Purpose: initialize a blur action's frame count and pass count.
 */
zFMV_ActionBlur::zFMV_ActionBlur(
    int framesRemainingParam,
    int blurPassCountParam
) {
    framesRemaining = framesRemainingParam;
    blurPassCount = blurPassCountParam;
}

/**
 * Reimplements 0x463870: zFMV_ActionBlur::Begin.
 * Purpose: capture active surface bounds and seed the blur source surface.
 */
void zFMV_ActionBlur::Begin(
    double
) {
    primarySurfaceRect.top = 0;
    swSurfaceRect.top = 0;
    primarySurfaceRect.left = 0;
    swSurfaceRect.left = 0;
    swSurfaceRect.right = zVideo::GetSwSurfaceWidth();
    swSurfaceRect.bottom = zVideo::GetSwSurfaceHeight();
    primarySurfaceRect.right = zVideo::GetPrimarySurfaceWidth();
    primarySurfaceRect.bottom = zVideo::GetPrimarySurfaceHeight();

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::Fx_SetSurfaceState(
            zVideo::GetPrimarySurfacePixels(),
            swSurfaceRect.right,
            swSurfaceRect.bottom,
            zVideo::GetPrimarySurfacePitch()
        );
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&primarySurfaceRect),
            (zVidRect32 *)(&swSurfaceRect)
        );
    } else {
        zVideo::Fx_SetSurfaceState(
            zVideo::GetSwSurfacePixels(),
            swSurfaceRect.right,
            swSurfaceRect.bottom,
            zVideo::GetSwSurfacePitch()
        );
        g_zVideo_pfnBltPrimaryToSwRectDirect(
            (zVidRect32 *)(&primarySurfaceRect),
            (zVidRect32 *)(&swSurfaceRect)
        );
    }
}

/**
 * Reimplements 0x463920: zFMV_ActionBlur::End.
 * Purpose: restore the video FX surface state to the primary surface.
 */
void zFMV_ActionBlur::End() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_ActionBlur virtual slot contract.
 * Purpose: run blur actions immediately without timed polling.
 */
void zFMV_ActionBlur::RunBlocking() {
    RunBlockingImmediate();
}

/**
 * Reimplements 0x463950: zFMV_ActionBlur::Update.
 * Purpose: apply combined blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlur::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeCombined
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeCombined
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}

/**
 * Reimplements 0x4639e0: zFMV_ActionBlurH::Update.
 * Purpose: apply horizontal blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlurH::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeHorizontal
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeHorizontal
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}

/**
 * Reimplements 0x463a70: zFMV_ActionBlurV::Update.
 * Purpose: apply vertical blur passes for one frame and report whether frames remain.
 */
int zFMV_ActionBlurV::Update(
    double
) {
    --framesRemaining;
    int passes = blurPassCount;

    if (g_zVideo_ActiveRendererPath != k_zFMV_RendererBackendSoftware) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeVertical
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockPrimarySurfaceState();
    } else {
        zVideo::RunPostprocessOnSwBuffer();
        if (passes-- != 0) {
            ++passes;
            do {
                zVideo::buff_BlurRegionByMode(
                    0,
                    k_zFMV_BlurModeVertical
                );
                --passes;
            } while (passes != 0);
        }
        zVideo::Dispatch_UnlockSwSurfaceState();
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&swSurfaceRect),
            (zVidRect32 *)(&primarySurfaceRect)
        );
    }

    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    return framesRemaining != 0;
}
/**
 * Reimplements 0x463b00: zFMV_ActionPlayMci::Constructor.
 * Purpose: build the MCI media path, create playback state, and set its destination rect.
 */
zFMV_ActionPlayMci::zFMV_ActionPlayMci(
    HWND hwnd,
    const char *mediaRootPath,
    const char *playbackTitle
) {
    mediaPath = (char *)(calloc(
        strlen(mediaRootPath) + strlen(playbackTitle) + 0x1b,
        1
    ));
    sprintf(
        mediaPath,
        "%s\\%s",
        mediaRootPath,
        playbackTitle
    );

    zFMV_Playback *const playbackObject = new zFMV_Playback(
        mediaPath,
        hwnd
    );
    playback = playbackObject;

    g_zFMV_ActionPlayMci_DestRect.top = 0;
    g_zFMV_ActionPlayMci_DestRect.left = 0;
    int discard;
    zRndr::GetActiveRegionState(
        &g_zFMV_ActionPlayMci_DestRect.right,
        &g_zFMV_ActionPlayMci_DestRect.bottom,
        &discard,
        &discard
    );
    playback->SetDestRect(&g_zFMV_ActionPlayMci_DestRect);
}

/**
 * Reimplements 0x463c10: zFMV_ActionPlayMci::~zFMV_ActionPlayMci.
 * Purpose: free MCI media/playback state.
 */
zFMV_ActionPlayMci::~zFMV_ActionPlayMci() {
    if (mediaPath != 0) {
        free(mediaPath);
        mediaPath = 0;
    }

    zFMV_Playback *const playbackObject = playback;
    if (playbackObject != 0) {
        playbackObject->Destructor();
        ::operator delete(playbackObject);
        playback = 0;
    }
}

/**
 * Reimplements 0x463c90: zFMV_ActionPlayMci::Update.
 * Purpose: report immediate completion for MCI playback update polling.
 */
int zFMV_ActionPlayMci::Update(
    double
) {
    return 0;
}

/**
 * Reimplements 0x463ca0: zFMV_ActionPlayMci::Begin.
 * Purpose: start the configured MCI playback if a playback object exists.
 */
void zFMV_ActionPlayMci::Begin(
    double
) {
    if (playback != 0) {
        playback->OpenAndPlay(
            0,
            -1,
            0
        );
    }
}

/**
 * Reimplements 0x463cc0: zFMV_ActionPlayMci::End.
 * Purpose: stop MCI playback while preserving and restoring the active video surface.
 */
void zFMV_ActionPlayMci::End() {
    zVideo::Dispatch_LockDisplayModeSurfaceState();
    zVidImagePartial *capturedImage = zVideo_buff_CaptureSurfaceToImage(2);
    zVideo::Dispatch_UnlockDisplayModeSurfaceState();

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            1
        );
    }

    if (playback != 0) {
        playback->StopAndClose();
    }

    if (capturedImage != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();
        zVid_Image::BlitToActiveTarget(
            capturedImage,
            0,
            0,
            0,
            0
        );
        zVideo::Dispatch_UnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(
            0,
            0,
            0,
            1
        );
        zVid_Image::ReleaseIfNotDefault(capturedImage);
    }
}

#include "fmv_stream.cpp"

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: provide the default no-op action start hook.
 */
void zFMV_Action::Begin(double) {}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: provide the default no-op action finish hook.
 */
void zFMV_Action::End() {}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in the zFMV_Action virtual slot contract.
 * Purpose: dispatch the default timed blocking action runner.
 */
void zFMV_Action::RunBlocking() {
    RunBlockingTimed();
}
