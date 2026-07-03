#include "zclass.h"

#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zModel/gmod.h"
#include "zdi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * Reimplements data 0x539cd8: g_GameZ_Zbd_NodeIndexScratch.
 * Evidence: parent-linked BN data name/type for a 4-byte writable pointer
 * followed by the capacity global at 0x539cdc.
 * Purpose: own the reusable ZBD node-reference conversion scratch buffer.
 */
zClass_NodePartial **g_GameZ_Zbd_NodeIndexScratch = 0;
RECOIL_STATIC_ASSERT(sizeof(g_GameZ_Zbd_NodeIndexScratch) == 4);
/**
 * Reimplements data 0x539cdc: g_GameZ_Zbd_NodeIndexScratchCapacity.
 * Evidence: parent-linked BN data name/type for the 4-byte entry count paired
 * with g_GameZ_Zbd_NodeIndexScratch.
 * Purpose: track the current pointer/index capacity of the ZBD scratch buffer.
 */
int g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
RECOIL_STATIC_ASSERT(sizeof(g_GameZ_Zbd_NodeIndexScratchCapacity) == 4);
/**
 * Reimplements data 0x4dee1c: g_zClass_SourceFile_ClsZbdC.
 * Purpose: preserve the legacy source-file literal for cls_zbd.c diagnostics.
 */
char g_zClass_SourceFile_ClsZbdC[0x25] =
    "D:\\Proj\\GameZRecoil\\zClass\\cls_zbd.c";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_SourceFile_ClsZbdC) == 0x25);
/**
 * Reimplements data 0x4dee44: g_zClass_WriteNodeDataErrorMsg.
 * Purpose: preserve the legacy write-node diagnostic literal.
 */
char g_zClass_WriteNodeDataErrorMsg[0x19] =
    "Error writing node data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeDataErrorMsg) == 0x19);
/**
 * Reimplements data 0x4dee60: g_zClass_WriteWorldAreaPartitionDataErrorMsg.
 * Purpose: preserve the legacy world-area write diagnostic literal.
 */
char g_zClass_WriteWorldAreaPartitionDataErrorMsg[0x29] =
    "Error writing world area partition data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteWorldAreaPartitionDataErrorMsg) == 0x29);
/**
 * Reimplements data 0x4dee8c: g_zClass_WriteNodeWorldDataErrorMsg.
 * Purpose: preserve the legacy write-node-world diagnostic literal.
 */
char g_zClass_WriteNodeWorldDataErrorMsg[0x1f] =
    "Error writing node world data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeWorldDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4deeac: g_zClass_WriteNodeWindowDataErrorMsg.
 * Purpose: preserve the legacy write-node-window diagnostic literal.
 */
char g_zClass_WriteNodeWindowDataErrorMsg[0x20] =
    "Error writing node window data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeWindowDataErrorMsg) == 0x20);
/**
 * Reimplements data 0x4deecc: g_zClass_WriteNodeDisplayDataErrorMsg.
 * Purpose: preserve the legacy write-node-display diagnostic literal.
 */
char g_zClass_WriteNodeDisplayDataErrorMsg[0x21] =
    "Error writing node display data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeDisplayDataErrorMsg) == 0x21);
/**
 * Reimplements data 0x4deef0: g_zClass_WriteNodeCameraDataErrorMsg.
 * Purpose: preserve the legacy write-node-camera diagnostic literal.
 */
char g_zClass_WriteNodeCameraDataErrorMsg[0x20] =
    "Error writing node camera data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeCameraDataErrorMsg) == 0x20);
/**
 * Reimplements data 0x4def10: g_zClass_WriteNodeLightDataErrorMsg.
 * Purpose: preserve the legacy write-node-light diagnostic literal.
 */
char g_zClass_WriteNodeLightDataErrorMsg[0x1f] =
    "Error writing node light data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeLightDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4def30: g_zClass_WriteNodeLodDataErrorMsg.
 * Purpose: preserve the legacy write-node-lod diagnostic literal.
 */
char g_zClass_WriteNodeLodDataErrorMsg[0x1d] =
    "Error writing node lod data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeLodDataErrorMsg) == 0x1d);
/**
 * Reimplements data 0x4def50: g_zClass_WriteNodeObject3DDataErrorMsg.
 * Purpose: preserve the legacy write-node-object3d diagnostic literal.
 */
char g_zClass_WriteNodeObject3DDataErrorMsg[0x22] =
    "Error writing node object3d data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeObject3DDataErrorMsg) == 0x22);
/**
 * Reimplements data 0x4def74: g_zClass_WriteNodeUnrecognizedClassTypeFmt.
 * Purpose: preserve the legacy write-node class-type diagnostic format.
 */
char g_zClass_WriteNodeUnrecognizedClassTypeFmt[0x4c] =
    "gClsWriteNode(): Unrecognized node class type:\n"
    "  node = %s class_type = %d\n";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeUnrecognizedClassTypeFmt) == 0x4c);
/**
 * Reimplements data 0x4defc0: g_zClass_WriteNodeSoundDataErrorMsg.
 * Purpose: preserve the legacy write-node-sound diagnostic literal.
 */
char g_zClass_WriteNodeSoundDataErrorMsg[0x1f] =
    "Error writing node sound data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteNodeSoundDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4defe0: g_zClass_WriteSoundNodeDataIncompleteMsg.
 * Purpose: preserve the legacy incomplete write-sound-node diagnostic literal.
 */
char g_zClass_WriteSoundNodeDataIncompleteMsg[0x31] =
    "Writing sound node data: Must complete software.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteSoundNodeDataIncompleteMsg) == 0x31);
/**
 * Reimplements data 0x4df014: g_zClass_WriteGameZNodeActionCallbackDefinedFmt.
 * Purpose: preserve the legacy write-node action-callback diagnostic format.
 */
char g_zClass_WriteGameZNodeActionCallbackDefinedFmt[0x38] =
    "Writing gamez.zbd; node %s has action callback defined.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteGameZNodeActionCallbackDefinedFmt) == 0x38);
/**
 * Reimplements data 0x4df04c: g_zClass_WriteGameZHeaderDataErrorMsg.
 * Purpose: preserve the legacy write-GameZ-header diagnostic literal.
 */
char g_zClass_WriteGameZHeaderDataErrorMsg[0x21] =
    "Error writing GameZ header data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_WriteGameZHeaderDataErrorMsg) == 0x21);
/**
 * Reimplements data 0x4df070: g_zClass_ZbdFilenameTooLongFmt.
 * Purpose: preserve the legacy zbd filename length diagnostic format.
 */
char g_zClass_ZbdFilenameTooLongFmt[0x37] =
    "zbd_filename length %d exceeds storage string size %d.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ZbdFilenameTooLongFmt) == 0x37);
/**
 * Reimplements data 0x4df0a8: g_zClass_ReadGameZNodeListErrorMsg.
 * Purpose: preserve the legacy read-node-list diagnostic literal.
 */
char g_zClass_ReadGameZNodeListErrorMsg[0x1f] =
    "Error reading GameZ Node list.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZNodeListErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4df0c8: g_zClass_ReadWorldAreaPartitionDataErrorMsg.
 * Purpose: preserve the legacy read-world-area diagnostic literal.
 */
char g_zClass_ReadWorldAreaPartitionDataErrorMsg[0x29] =
    "Error reading world area partition data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadWorldAreaPartitionDataErrorMsg) == 0x29);
/**
 * Reimplements data 0x4df0f4: g_zClass_ReadNodeUnrecognizedClassTypeFmt.
 * Purpose: preserve the legacy read-node class-type diagnostic format.
 */
char g_zClass_ReadNodeUnrecognizedClassTypeFmt[0x4b] =
    "gClsReadNode(): Unrecognized node class type:\n"
    "  node = %s class_type = %d\n";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeUnrecognizedClassTypeFmt) == 0x4b);
/**
 * Reimplements data 0x4df140: g_zClass_ReadNodeWorldDataErrorMsg.
 * Purpose: preserve the legacy read-node-world diagnostic literal.
 */
char g_zClass_ReadNodeWorldDataErrorMsg[0x1f] =
    "Error reading node world data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeWorldDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4df160: g_zClass_ReadNodeWindowDataErrorMsg.
 * Purpose: preserve the legacy read-node-window diagnostic literal.
 */
char g_zClass_ReadNodeWindowDataErrorMsg[0x20] =
    "Error reading node window data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeWindowDataErrorMsg) == 0x20);
/**
 * Reimplements data 0x4df180: g_zClass_ReadNodeDisplayDataErrorMsg.
 * Purpose: preserve the legacy read-node-display diagnostic literal.
 */
char g_zClass_ReadNodeDisplayDataErrorMsg[0x21] =
    "Error reading node display data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeDisplayDataErrorMsg) == 0x21);
/**
 * Reimplements data 0x4df1a4: g_zClass_ReadNodeCameraDataErrorMsg.
 * Purpose: preserve the legacy read-node-camera diagnostic literal.
 */
char g_zClass_ReadNodeCameraDataErrorMsg[0x20] =
    "Error reading node camera data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeCameraDataErrorMsg) == 0x20);
/**
 * Reimplements data 0x4df1c4: g_zClass_ReadNodeLightDataErrorMsg.
 * Purpose: preserve the legacy read-node-light diagnostic literal.
 */
char g_zClass_ReadNodeLightDataErrorMsg[0x1f] =
    "Error reading node light data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeLightDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4df1e4: g_zClass_ReadNodeLodDataErrorMsg.
 * Purpose: preserve the legacy read-node-lod diagnostic literal.
 */
char g_zClass_ReadNodeLodDataErrorMsg[0x1d] =
    "Error reading node lod data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeLodDataErrorMsg) == 0x1d);
/**
 * Reimplements data 0x4df204: g_zClass_ReadNodeObject3DDataErrorMsg.
 * Purpose: preserve the legacy read-node-object3d diagnostic literal.
 */
char g_zClass_ReadNodeObject3DDataErrorMsg[0x22] =
    "Error reading node object3d data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeObject3DDataErrorMsg) == 0x22);
/**
 * Reimplements data 0x4df228: g_zClass_ReadNodeSoundDataErrorMsg.
 * Purpose: preserve the legacy read-node-sound diagnostic literal.
 */
char g_zClass_ReadNodeSoundDataErrorMsg[0x1f] =
    "Error reading node sound data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadNodeSoundDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4df248: g_zClass_ReadSoundNodeDataIncompleteMsg.
 * Purpose: preserve the legacy incomplete read-sound-node diagnostic literal.
 */
char g_zClass_ReadSoundNodeDataIncompleteMsg[0x31] =
    "Reading sound node data: Must complete software.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadSoundNodeDataIncompleteMsg) == 0x31);
/**
 * Reimplements data 0x4df27c: g_zClass_ReadGameZNodeBufferErrorMsg.
 * Purpose: preserve the legacy read-node-buffer diagnostic literal.
 */
char g_zClass_ReadGameZNodeBufferErrorMsg[0x21] =
    "Error reading GameZ Node buffer.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZNodeBufferErrorMsg) == 0x21);
/**
 * Reimplements data 0x4df2a0: g_zClass_ReadGameZNodeDataErrorMsg.
 * Purpose: preserve the legacy read-GameZ-node diagnostic literal.
 */
char g_zClass_ReadGameZNodeDataErrorMsg[0x1f] =
    "Error reading GameZ node data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZNodeDataErrorMsg) == 0x1f);
/**
 * Reimplements data 0x4df2c0: g_zClass_ReadGameZModel3DDataErrorMsg.
 * Purpose: preserve the legacy read-GameZ-model3d diagnostic literal.
 */
char g_zClass_ReadGameZModel3DDataErrorMsg[0x22] =
    "Error reading GameZ model3d data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZModel3DDataErrorMsg) == 0x22);
/**
 * Reimplements data 0x4df2e4: g_zClass_ReadGameZMaterialDataErrorMsg.
 * Purpose: preserve the legacy read-GameZ-material diagnostic literal.
 */
char g_zClass_ReadGameZMaterialDataErrorMsg[0x23] =
    "Error reading GameZ material data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZMaterialDataErrorMsg) == 0x23);
/**
 * Reimplements data 0x4df308: g_zClass_ReadGameZTextureDataErrorMsg.
 * Purpose: preserve the legacy read-GameZ-texture diagnostic literal.
 */
char g_zClass_ReadGameZTextureDataErrorMsg[0x22] =
    "Error reading GameZ texture data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZTextureDataErrorMsg) == 0x22);
/**
 * Reimplements data 0x4df32c: g_zClass_ReadGameZHeaderIncompatibleVersionMsg.
 * Purpose: preserve the legacy incompatible-version read-header diagnostic literal.
 */
char g_zClass_ReadGameZHeaderIncompatibleVersionMsg[0x3b] =
    "Error reading GameZ header data; incompatible file version";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZHeaderIncompatibleVersionMsg) == 0x3b);
/**
 * Reimplements data 0x4df368: g_zClass_ReadGameZHeaderIncompatibleTypeMsg.
 * Purpose: preserve the legacy incompatible-type read-header diagnostic literal.
 */
char g_zClass_ReadGameZHeaderIncompatibleTypeMsg[0x38] =
    "Error reading GameZ header data; incompatible file type";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZHeaderIncompatibleTypeMsg) == 0x38);
/**
 * Reimplements data 0x4df3a0: g_zClass_ReadGameZHeaderDataErrorMsg.
 * Purpose: preserve the legacy read-GameZ-header diagnostic literal.
 */
char g_zClass_ReadGameZHeaderDataErrorMsg[0x21] =
    "Error reading GameZ header data.";
RECOIL_STATIC_ASSERT(sizeof(g_zClass_ReadGameZHeaderDataErrorMsg) == 0x21);
}

namespace {
    const int kZClassNodeCamera = 1;
    const int kZClassNodeWorld = 2;
    const int kZClassNodeWindow = 3;
    const int kZClassNodeDisplay = 4;
    const int kZClassNodeObject3D = 5;
    const int kZClassNodeLod = 6;
    const int kZClassNodeLight = 9;
    const int kZClassNodeSound = 10;

    /**
     * Original inline/static helper; no standalone retail function is known.
     * Observed callers are 0x454a50 and 0x4544b0 in this owner.
     * Evidence: repeated BN/source-file zError::ReportOld write-failure
     * callsites share the cls_zbd.c source literal and return -1.
     * Purpose: report a ZBD write failure and return the caller's error code.
     */
    inline int ReportZbdWriteFailure(
        int sourceLine,
        const char *message
    ) {
        zError::ReportOld(
            0x200,
            g_zClass_SourceFile_ClsZbdC,
            sourceLine,
            message
        );
        return -1;
    }

    /**
     * Original inline/static helper; no standalone retail function is known.
     * Observed caller is 0x454c60 in this owner.
     * Evidence: repeated BN/source-file zError::ReportOld read-failure
     * callsites share the cls_zbd.c source literal and return -1.
     * Purpose: report a ZBD read failure and return the caller's error code.
     */
    inline int ReportZbdReadFailure(
        int sourceLine,
        const char *message
    ) {
        zError::ReportOld(
            0x200,
            g_zClass_SourceFile_ClsZbdC,
            sourceLine,
            message
        );
        return -1;
    }

    /**
     * Original inline/static helper; no standalone retail function is known.
     * Observed caller is 0x4544b0 in this owner.
     * Evidence: BN/source-file write-node branches use identical fwrite
     * element-count checks for serialized class-data blobs.
     * Purpose: write one fixed-size ZBD blob to the active FILE stream.
     */
    inline bool WriteZbdBlob(
        const void *data,
        size_t byteCount,
        void *stream
    ) {
        return fwrite(
            data,
            byteCount,
            1,
            (FILE *)(stream)
        ) == 1;
    }

    /**
     * Original inline/static helper; no standalone retail function is known.
     * Observed caller is 0x454c60 in this owner.
     * Evidence: BN/source-file read-node branches use identical fread
     * element-count checks for serialized class-data blobs.
     * Purpose: read one fixed-size ZBD blob from the active FILE stream.
     */
    inline bool ReadZbdBlob(
        void *data,
        size_t byteCount,
        void *stream
    ) {
        return fread(
            data,
            byteCount,
            1,
            (FILE *)(stream)
        ) == 1;
    }
}

namespace GameZ {
    /**
     * Reimplements 0x454a50: GameZ::WriteZBDFile.
     * Evidence: BN name/source-file comment and cls_zbd.c callees serialize
     * the header, texture directory, material pool, DI pool, and node table.
     * Purpose: write a GameZ ZBD archive and patch the header offsets.
     */
    RECOIL_NO_GS int __fastcall WriteZBDFile(const char *filename) {
        const size_t filenameLength = strlen(filename);
        if (filenameLength == 0) {
            return -1;
        }

        if (filenameLength < 0x2f) {
            memcpy(
                g_zClass_CurrentZbdPath,
                filename,
                filenameLength + 1
            );
        } else {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x272,
                g_zClass_ZbdFilenameTooLongFmt,
                (int)(filenameLength),
                0x30
            );
        }

        FILE *const file = fopen(
            filename,
            "wb"
        );
        if (file == 0) {
            return -1;
        }

        zClass_ZbdHeader header;
        header.magic = 0x02971222;
        header.version = 0x0f;
        if (fwrite(
            &header,
            sizeof(header),
            1,
            file
        ) != 1) {
            return ReportZbdWriteFailure(
                0x285,
                g_zClass_WriteGameZHeaderDataErrorMsg
            );
        }

        header.texDirOffset = ftell(file);
        header.texDirArg = zImage::WriteTextureDirectory(file);
        header.matlOffset = ftell(file);
        zModel_MatlBuffer::WriteGameZ(file);
        header.model3dOffset = ftell(file);
        zModel_DiPool::WriteToStream(file);
        zClass_NodeList::ProcessPendingFrees();
        header.nodeFreeHead = g_zClass_NodeFreeHeadIndex;
        header.nodeTableOffset = ftell(file);
        header.nodeCount = GameZ_ZBD::WriteNodeTable(file);

        fseek(
            file,
            0,
            SEEK_SET
        );
        if (fwrite(
            &header,
            sizeof(header),
            1,
            file
        ) != 1) {
            return ReportZbdWriteFailure(
                0x2aa,
                g_zClass_WriteGameZHeaderDataErrorMsg
            );
        }

        fclose(file);
        return 0;
    }

    /**
     * Reimplements 0x455520: GameZ::ReadZBDFile.
     * Evidence: BN name/source-file comment and cls_zbd.c callee order reload
     * texture, material, model, and node-table sections from header offsets.
     * Purpose: read a GameZ ZBD archive into the engine resource state.
     */
    RECOIL_NO_GS int __fastcall ReadZBDFile(const char *filename) {
        const size_t filenameLength = strlen(filename);
        if (filenameLength == 0) {
            return -1;
        }

        if (filenameLength < 0x2f) {
            memcpy(
                g_zClass_CurrentZbdPath,
                filename,
                filenameLength + 1
            );
        } else {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x551,
                g_zClass_ZbdFilenameTooLongFmt,
                (int)(filenameLength),
                0x30
            );
        }

        zClass_ZbdHeader header;
        FILE *const file = OpenAndReadZBDHeader(
            filename,
            &header
        );
        if (file == 0) {
            return -1;
        }

        int sourceLine = 0;
        const char *message = 0;

        fseek(
            file,
            header.texDirOffset,
            SEEK_SET
        );
        if (zImage::ReadTextureDirectory(
            header.texDirArg,
            file
        ) < 0) {
            sourceLine = 0x562;
            message = g_zClass_ReadGameZTextureDataErrorMsg;
            goto readError;
        }

        fseek(
            file,
            header.matlOffset,
            SEEK_SET
        );
        if (zModel_MatlBuffer::ReadGameZ(file) < 0) {
            sourceLine = 0x56e;
            message = g_zClass_ReadGameZMaterialDataErrorMsg;
            goto readError;
        }

        fseek(
            file,
            header.model3dOffset,
            SEEK_SET
        );
        if (zModel_DiPool::ReadFromStream(file) < 0) {
            sourceLine = 0x57a;
            message = g_zClass_ReadGameZModel3DDataErrorMsg;
            goto readError;
        }

        fseek(
            file,
            header.nodeTableOffset,
            SEEK_SET
        );
        if (GameZ_ZBD::ReadNodeTable(
            header.nodeCount,
            file
        ) < 0) {
            sourceLine = 0x586;
            message = g_zClass_ReadGameZNodeDataErrorMsg;
            goto readError;
        }

        g_zClass_NodeFreeHeadIndex = header.nodeFreeHead;
        fclose(file);
        return 0;

    readError:
        zError::ReportOld(
            0x200,
            g_zClass_SourceFile_ClsZbdC,
            sourceLine,
            message
        );
        fclose(file);
        return -1;
    }

    /**
     * Reimplements 0x4556a0: GameZ::OpenAndReadZBDHeader.
     * Evidence: BN name/source-file comment and callers 0x455520/0x455730 use
     * this shared header validation before reading ZBD sections.
     * Purpose: open a ZBD file, read its header, and reject bad magic/version.
     */
    FILE *__fastcall OpenAndReadZBDHeader(
        const char *filename,
        zClass_ZbdHeader *outHeader
    ) {
        FILE *file = fopen(
            filename,
            "rb"
        );
        if (file == 0) {
            return 0;
        }

        int sourceLine = 0;
        const char *message = 0;
        if (fread(
            outHeader,
            sizeof(zClass_ZbdHeader),
            1,
            file
        ) != 1) {
            message = g_zClass_ReadGameZHeaderDataErrorMsg;
            sourceLine = 0x515;
        } else if (outHeader->magic != 0x02971222) {
            message = g_zClass_ReadGameZHeaderIncompatibleTypeMsg;
            sourceLine = 0x51e;
        } else if (outHeader->version != 0x0f) {
            message = g_zClass_ReadGameZHeaderIncompatibleVersionMsg;
            sourceLine = 0x527;
        } else {
            return file;
        }

        zError::ReportOld(
            0x200,
            g_zClass_SourceFile_ClsZbdC,
            sourceLine,
            message
        );
        fclose(file);
        return 0;
    }
}

namespace zClass {
    /**
     * Reimplements 0x4543a0: zClass::NodePtrToValidatedIndex
     * Purpose: convert a node-array pointer to its index only when the slot is
     * marked live in the ZBD node table.
     */
    int __fastcall NodePtrToValidatedIndex(zClass_NodePartial * node) {
        const int index = GameZ_ZBD::NodePtrToIndex(node);
        if (index >= 0 && (g_zClass_NodeArray[index].freeTag & 0x01000000u) != 0) {
            return index;
        }

        return -1;
    }
}

namespace GameZ_ZBD {
    /**
     * Reimplements 0x454370: GameZ_ZBD::NodePtrToIndex
     * Purpose: convert a node pointer in the ZBD node table to its slot index.
     */
    int __fastcall NodePtrToIndex(zClass_NodePartial * node) {
        if (node == 0) {
            return -1;
        }

        return (int)((zClass_NodeFreeListSlot *)(node)-g_zClass_NodeArray);
    }

    /**
     * Reimplements 0x4543d0: GameZ_ZBD::NodeIndexToPtr
     * Purpose: convert a non-negative ZBD node table index back to its node
     * pointer.
     */
    zClass_NodePartial *__fastcall NodeIndexToPtr(int index) {
        if (index < 0) {
            return 0;
        }

        return &g_zClass_NodeArray[index].node;
    }

    /**
     * Reimplements 0x4543f0: GameZ_ZBD::WriteNodeRefListIndices.
     * Evidence: BN name/source-file comment and write-node callers convert node
     * pointer lists through the shared scratch buffer before fwrite.
     * Purpose: serialize a node-reference list as node-table indices.
     */
    int __fastcall WriteNodeRefListIndices(
        zClass_NodePartial * *nodeRefList,
        int entryCount,
        void *stream
    ) {
        if (entryCount == 0) {
            return 0;
        }

        const size_t byteCount = (size_t)(entryCount) * sizeof(unsigned int);
        if (entryCount > g_GameZ_Zbd_NodeIndexScratchCapacity) {
            g_GameZ_Zbd_NodeIndexScratch =
                (zClass_NodePartial **)(realloc(
                    g_GameZ_Zbd_NodeIndexScratch,
                    byteCount
                ));
            g_GameZ_Zbd_NodeIndexScratchCapacity = entryCount;
        }

        memcpy(
            g_GameZ_Zbd_NodeIndexScratch,
            nodeRefList,
            byteCount
        );
        int *indices = (int *)(g_GameZ_Zbd_NodeIndexScratch);
        for (int i = 0; i < entryCount; ++i) {
            indices[i] = NodePtrToIndex(g_GameZ_Zbd_NodeIndexScratch[i]);
        }

        if (fwrite(
            g_GameZ_Zbd_NodeIndexScratch,
            byteCount,
            1,
            (FILE *)(stream)
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0xd7,
                g_zClass_WriteNodeDataErrorMsg
            );
            return -1;
        }

        return 0;
    }

    /**
     * Reimplements 0x4544b0: GameZ_ZBD::WriteSingleNodeClassData.
     * Evidence: BN name/source-file comment and class-id switch serialize the
     * node class payloads and nested node-reference lists.
     * Purpose: write one node's class-specific ZBD payload.
     */
    RECOIL_NO_GS int __fastcall WriteSingleNodeClassData(
        zClass_NodePartial * node,
        void *stream
    ) {
        int result = 0;
        if (node->actionCallback != 0) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0xf1,
                g_zClass_WriteGameZNodeActionCallbackDefinedFmt,
                node->name
            );
        }

        switch (node->classId) {
        case 0:
            break;

        case kZClassNodeSound: {
            result = 1;
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0xfc,
                g_zClass_WriteSoundNodeDataIncompleteMsg
            );

            zClass_SoundDataPartial *data = (zClass_SoundDataPartial *)(node->classData);
            if (!WriteZbdBlob(
                data,
                sizeof(zClass_SoundDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x105,
                    g_zClass_WriteNodeSoundDataErrorMsg
                );
            }

            if (data->attachedWorldCount > 0) {
                WriteNodeRefListIndices(
                    data->attachedWorlds,
                    data->attachedWorldCount,
                    stream
                );
            }
            break;
        }

        case kZClassNodeObject3D:
            result = 1;
            if (!WriteZbdBlob(
                node->classData,
                sizeof(zClass_Object3DDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x119,
                    g_zClass_WriteNodeObject3DDataErrorMsg
                );
            }
            break;

        case kZClassNodeLod:
            result = 1;
            if (!WriteZbdBlob(
                node->classData,
                sizeof(zClass_LodDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x128,
                    g_zClass_WriteNodeLodDataErrorMsg
                );
            }
            break;

        case kZClassNodeLight: {
            result = 1;
            zClass_LightDataPartial *data = (zClass_LightDataPartial *)(node->classData);
            if (!WriteZbdBlob(
                data,
                sizeof(zClass_LightDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x137,
                    g_zClass_WriteNodeLightDataErrorMsg
                );
            }

            if (data->attachedWorldCount > 0) {
                WriteNodeRefListIndices(
                    data->attachedWorlds,
                    data->attachedWorldCount,
                    stream
                );
            }
            break;
        }

        case kZClassNodeCamera: {
            result = 1;
            zClass_CameraDataPartial data;
            memcpy(
                &data,
                node->classData,
                sizeof(data)
            );
            data.worldNode = (zClass_NodePartial *)((int)(NodePtrToIndex(data.worldNode)));
            data.windowNode = (zClass_NodePartial *)((int)(NodePtrToIndex(data.windowNode)));
            data.horizonNode = (zClass_NodePartial *)((int)(NodePtrToIndex(data.horizonNode)));
            data.horizonXZNode = (zClass_NodePartial *)((int)(NodePtrToIndex(data.horizonXZNode)));

            if (!WriteZbdBlob(
                &data,
                sizeof(data),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x15a,
                    g_zClass_WriteNodeCameraDataErrorMsg
                );
            }
            break;
        }

        case kZClassNodeDisplay:
            result = 1;
            if (!WriteZbdBlob(
                node->classData,
                sizeof(zClass_DisplayDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x16a,
                    g_zClass_WriteNodeDisplayDataErrorMsg
                );
            }
            break;

        case kZClassNodeWindow:
            result = 1;
            if (!WriteZbdBlob(
                node->classData,
                sizeof(zClass_WindowDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x179,
                    g_zClass_WriteNodeWindowDataErrorMsg
                );
            }
            break;

        case kZClassNodeWorld: {
            result = 1;
            zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(node->classData);
            if (!WriteZbdBlob(
                data,
                sizeof(zClass_WorldDataPartial),
                stream
            )) {
                return ReportZbdWriteFailure(
                    0x18c,
                    g_zClass_WriteNodeWorldDataErrorMsg
                );
            }

            if (data->lightCount > 0) {
                WriteNodeRefListIndices(
                    data->lightNodes,
                    data->lightCount,
                    stream
                );
            }
            if (data->soundCount > 0) {
                WriteNodeRefListIndices(
                    data->soundNodes,
                    data->soundCount,
                    stream
                );
            }

            {
                for (int row = 0; row < data->areaGridRowCount; ++row) {
                    zWorldAreaPartial *area = data->areaGridRows[row];
                    {
                        for (int col = 0; col < data->areaGridColCount; ++col) {
                            if (!WriteZbdBlob(
                                area,
                                sizeof(zWorldAreaPartial),
                                stream
                            )) {
                                return ReportZbdWriteFailure(
                                    0x1a8,
                                    g_zClass_WriteWorldAreaPartitionDataErrorMsg
                                );
                            }

                            if (area->childCount > 0) {
                                WriteNodeRefListIndices(
                                    area->childList,
                                    area->childCount,
                                    stream
                                );
                            }
                            ++area;
                        }
                    }
                }
            }
            break;
        }

        default:
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsZbdC,
                0x1bd,
                g_zClass_WriteNodeUnrecognizedClassTypeFmt,
                node->name,
                node->classId
            );
            return 0;
        }

        if (node->listCountA > 0) {
            result = 1;
            WriteNodeRefListIndices(
                node->listA,
                node->listCountA,
                stream
            );
        }
        if (node->listCountB > 0) {
            result = 1;
            WriteNodeRefListIndices(
                node->listB,
                node->listCountB,
                stream
            );
        }

        return result;
    }

    /**
     * Reimplements 0x454890: GameZ_ZBD::WriteNodeTable.
     * Evidence: BN name/source-file comment and cls_zbd.c writes copy the node
     * array, append payloads, and patch encoded class-data offsets.
     * Purpose: serialize the live ZBD node table and associated payload blocks.
     */
    int __fastcall WriteNodeTable(void *stream) {
        int result = g_zClass_NodeArraySize;
        if (result == 0) {
            return result;
        }

        const int byteCount = result * (int)(sizeof(zClass_NodeFreeListSlot));
        zClass_NodeFreeListSlot *nodeBuffer = (zClass_NodeFreeListSlot *)(malloc(byteCount));
        memcpy(
            nodeBuffer,
            g_zClass_NodeArray,
            byteCount
        );

        for (int i = 0; i < result; ++i) {
            zClass_NodePartial *node = &nodeBuffer[i].node;
            if (node->listCountA == 0) {
                node->listA = 0;
            }
            if (node->listCountB == 0) {
                node->listB = 0;
            }
            node->userDataOrDiRef = (unsigned int)(zDi::PtrToIndexOrMinus1(
                (zDiPartial *)((unsigned int)(node->userDataOrDiRef))
            ));
            node->actionCallback = 0;
        }

        FILE *file = (FILE *)(stream);
        const long nodeTableOffset = ftell(file);
        if (fwrite(
            nodeBuffer,
            byteCount,
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x218,
                g_zClass_WriteNodeDataErrorMsg
            );
            result = 0;
        }

        if (result > 0) {
            for (int i = 0; i < result; ++i) {
                const long classDataOffset = ftell(file);
                if (WriteSingleNodeClassData(
                    &nodeBuffer[i].node,
                    stream
                ) != 0) {
                    const unsigned int freeTag = nodeBuffer[i].freeTag;
                    nodeBuffer[i].freeTag =
                        (((unsigned int)(classDataOffset) ^ freeTag) & 0x00ffffffu) ^ freeTag;
                }
            }
        }

        const long endOffset = ftell(file);
        fseek(
            file,
            nodeTableOffset,
            SEEK_SET
        );
        if (fwrite(nodeBuffer, g_zClass_NodeArraySize * sizeof(zClass_NodeFreeListSlot), 1, file) !=
            1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x23a,
                g_zClass_WriteNodeDataErrorMsg
            );
            result = 0;
        }
        fseek(
            file,
            endOffset,
            SEEK_SET
        );

        if (g_GameZ_Zbd_NodeIndexScratch != 0) {
            free(g_GameZ_Zbd_NodeIndexScratch);
            g_GameZ_Zbd_NodeIndexScratch = 0;
        }
        g_GameZ_Zbd_NodeIndexScratchCapacity = 0;

        free(nodeBuffer);
        return result;
    }

    /**
     * Reimplements 0x454bf0: GameZ_ZBD::ReadNodeRefListIndices.
     * Evidence: BN name/source-file comment and read-node callers read integer
     * indices into the destination list before resolving node pointers.
     * Purpose: deserialize a node-reference list from node-table indices.
     */
    int __fastcall ReadNodeRefListIndices(
        zClass_NodePartial * *nodeRefList,
        int entryCount,
        void *stream
    ) {
        if (entryCount == 0) {
            return 0;
        }

        const size_t byteCount = (size_t)(entryCount) * sizeof(unsigned int);
        if (fread(
            nodeRefList,
            byteCount,
            1,
            (FILE *)(stream)
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x2d0,
                g_zClass_ReadGameZNodeListErrorMsg
            );
            return -1;
        }

        for (int i = 0; i < entryCount; ++i) {
            const int index = (int)((int)(nodeRefList[i]));
            nodeRefList[i] = NodeIndexToPtr(index);
        }

        return 0;
    }

    /**
     * Reimplements 0x454c60: GameZ_ZBD::ReadSingleNodeClassData.
     * Evidence: BN name/source-file comment and class-id switch allocate/read
     * the node class payloads, node-reference lists, and type-list entries.
     * Purpose: read one node's class-specific ZBD payload.
     */
    int __fastcall ReadSingleNodeClassData(
        zClass_NodePartial * node,
        void *stream
    ) {
        int result = 0;
        switch (node->classId) {
        case 0:
            break;

        case kZClassNodeSound: {
            result = 1;
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x2f1,
                g_zClass_ReadSoundNodeDataIncompleteMsg
            );

            zClass_SoundDataPartial *data =
                (zClass_SoundDataPartial *)(malloc(sizeof(zClass_SoundDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(
                data,
                sizeof(zClass_SoundDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x2fc,
                    g_zClass_ReadNodeSoundDataErrorMsg
                );
            }

            data->sample = 0;
            data->playHandle = 0;
            if (data->attachedWorldCount > 0) {
                data->attachedWorlds = (zClass_NodePartial **)(malloc(
                    data->attachedWorldCount * sizeof(zClass_NodePartial *)
                ));
                ReadNodeRefListIndices(
                    data->attachedWorlds,
                    data->attachedWorldCount,
                    stream
                );
            } else {
                data->attachedWorlds = 0;
            }

            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                0x0a,
                node
            );
            break;
        }

        case kZClassNodeObject3D:
            result = 1;
            node->classData = malloc(sizeof(zClass_Object3DDataPartial));
            if (!ReadZbdBlob(
                node->classData,
                sizeof(zClass_Object3DDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x323,
                    g_zClass_ReadNodeObject3DDataErrorMsg
                );
            }
            zClass_TypeList::Insert(
                6,
                node
            );
            break;

        case kZClassNodeLod:
            result = 1;
            node->classData = malloc(sizeof(zClass_LodDataPartial));
            if (!ReadZbdBlob(
                node->classData,
                sizeof(zClass_LodDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x338,
                    g_zClass_ReadNodeLodDataErrorMsg
                );
            }
            zClass_TypeList::Insert(
                6,
                node
            );
            break;

        case kZClassNodeLight: {
            result = 1;
            zClass_LightDataPartial *data =
                (zClass_LightDataPartial *)(malloc(sizeof(zClass_LightDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(
                data,
                sizeof(zClass_LightDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x34d,
                    g_zClass_ReadNodeLightDataErrorMsg
                );
            }

            if (data->attachedWorldCount > 0) {
                data->attachedWorlds = (zClass_NodePartial **)(malloc(
                    data->attachedWorldCount * sizeof(zClass_NodePartial *)
                ));
                ReadNodeRefListIndices(
                    data->attachedWorlds,
                    data->attachedWorldCount,
                    stream
                );
            } else {
                data->attachedWorlds = 0;
            }

            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                9,
                node
            );
            break;
        }

        case kZClassNodeCamera: {
            result = 1;
            zClass_CameraDataPartial *data =
                (zClass_CameraDataPartial *)(malloc(sizeof(zClass_CameraDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(
                data,
                sizeof(zClass_CameraDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x371,
                    g_zClass_ReadNodeCameraDataErrorMsg
                );
            }

            data->worldNode = NodeIndexToPtr((int)((int)(data->worldNode)));
            data->windowNode = NodeIndexToPtr((int)((int)(data->windowNode)));
            data->horizonNode = NodeIndexToPtr((int)((int)(data->horizonNode)));
            data->horizonXZNode = NodeIndexToPtr((int)((int)(data->horizonXZNode)));

            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                8,
                node
            );
            zClass_Camera::gwCameraSetNearFarClip(
                node,
                data->nearClip,
                data->farClip
            );
            zClass_Camera::gwCameraSetViewport(
                node,
                data->viewportWidth,
                data->viewportHeight
            );
            break;
        }

        case kZClassNodeDisplay: {
            result = 1;
            zClass_DisplayDataPartial *data =
                (zClass_DisplayDataPartial *)(malloc(sizeof(zClass_DisplayDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(
                data,
                sizeof(zClass_DisplayDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x39a,
                    g_zClass_ReadNodeDisplayDataErrorMsg
                );
            }

            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                0x0f,
                node
            );
            zClass_Display::gwDisplaySetBackgroundColor(
                node,
                data->backgroundR,
                data->backgroundG,
                data->backgroundB
            );
            break;
        }

        case kZClassNodeWindow:
            result = 1;
            node->classData = malloc(sizeof(zClass_WindowDataPartial));
            if (!ReadZbdBlob(
                node->classData,
                sizeof(zClass_WindowDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x3b7,
                    g_zClass_ReadNodeWindowDataErrorMsg
                );
            }
            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                0x0e,
                node
            );
            break;

        case kZClassNodeWorld: {
            result = 1;
            zClass_WorldDataPartial *data =
                (zClass_WorldDataPartial *)(malloc(sizeof(zClass_WorldDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(
                data,
                sizeof(zClass_WorldDataPartial),
                stream
            )) {
                return ReportZbdReadFailure(
                    0x3d4,
                    g_zClass_ReadNodeWorldDataErrorMsg
                );
            }

            if (data->lightCount > 0) {
                data->lightNodes = (zClass_NodePartial **)(malloc(
                    data->lightCount * sizeof(zClass_NodePartial *)
                ));
                ReadNodeRefListIndices(
                    data->lightNodes,
                    data->lightCount,
                    stream
                );
                data->lightDataList = (zClass_LightDataPartial **)(malloc(
                    data->lightCount * sizeof(zClass_LightDataPartial *)
                ));
            } else {
                data->lightNodes = 0;
                data->lightDataList = 0;
            }

            if (data->soundCount > 0) {
                data->soundNodes = (zClass_NodePartial **)(malloc(
                    data->soundCount * sizeof(zClass_NodePartial *)
                ));
                ReadNodeRefListIndices(
                    data->soundNodes,
                    data->soundCount,
                    stream
                );
                data->soundDataList = (zClass_SoundDataPartial **)(malloc(
                    data->soundCount * sizeof(zClass_SoundDataPartial *)
                ));
            } else {
                data->soundNodes = 0;
                data->soundDataList = 0;
            }

            data->areaGridRows =
                (zWorldAreaPartial **)(calloc(
                    data->areaGridRowCount,
                    sizeof(zWorldAreaPartial *)
                ));
            {
                for (int row = 0; row < data->areaGridRowCount; ++row) {
                    data->areaGridRows[row] = (zWorldAreaPartial *)(calloc(
                        data->areaGridColCount,
                        sizeof(zWorldAreaPartial)
                    ));
                }
            }

            {
                for (int row = 0; row < data->areaGridRowCount; ++row) {
                    zWorldAreaPartial *area = data->areaGridRows[row];
                    {
                        for (int col = 0; col < data->areaGridColCount; ++col) {
                            if (!ReadZbdBlob(
                                area,
                                sizeof(zWorldAreaPartial),
                                stream
                            )) {
                                return ReportZbdReadFailure(
                                    0x423,
                                    g_zClass_ReadWorldAreaPartitionDataErrorMsg
                                );
                            }

                            if (area->childCount > 0) {
                                area->childList = (zClass_NodePartial **)(malloc(
                                    area->childCount * sizeof(zClass_NodePartial *)
                                ));
                                ReadNodeRefListIndices(
                                    area->childList,
                                    area->childCount,
                                    stream
                                );
                            } else {
                                area->childList = 0;
                            }
                            ++area;
                        }
                    }
                }
            }

            data->pendingAreaUpdateCount = 0;
            data->pendingAreaUpdateCapacity = 0;
            data->pendingAreaUpdates = 0;
            zClass_TypeList::Insert(
                6,
                node
            );
            zClass_TypeList::Insert(
                0x0d,
                node
            );
            zClass_World::SetPendingFogState(
                node,
                data->fogState
            );
            zClass_World::SetPendingFogColorRgb01(
                node,
                data->ambientColor.red,
                data->ambientColor.green,
                data->ambientColor.blue
            );
            zClass_World::SetPendingFogAltitudeRange(
                node,
                data->fogHeightLow,
                data->fogHeightHigh
            );
            zClass_World::SetPendingFogRange(
                node,
                data->fogDistanceStart,
                data->fogDistanceEnd
            );
            zClass_World::SetPendingFogDensity(
                node,
                data->fogDensity
            );
            zClass_World::ApplyPendingFogSettings(node);
            break;
        }

        default:
            zError::ReportOld(
                0x400,
                g_zClass_SourceFile_ClsZbdC,
                0x45d,
                g_zClass_ReadNodeUnrecognizedClassTypeFmt,
                node->name,
                node->classId
            );
            return -1;
        }

        if (node->listCountA > 0) {
            result = 1;
            node->listA =
                (zClass_NodePartial **)(malloc(node->listCountA * sizeof(zClass_NodePartial *)));
            ReadNodeRefListIndices(
                node->listA,
                node->listCountA,
                stream
            );
        } else {
            node->listA = 0;
        }

        if (node->listCountB > 0) {
            node->listB =
                (zClass_NodePartial **)(malloc(node->listCountB * sizeof(zClass_NodePartial *)));
            ReadNodeRefListIndices(
                node->listB,
                node->listCountB,
                stream
            );
            return 1;
        }

        node->listB = 0;
        return result;
    }

    /**
     * Reimplements 0x455350: GameZ_ZBD::ReadNodeTable.
     * Evidence: BN name/source-file comment and cls_zbd.c reload path read the
     * node slots, rebuild class payloads, and reconnect world light/sound data.
     * Purpose: deserialize the ZBD node table into the runtime node array.
     */
    int __fastcall ReadNodeTable(
        int nodeCount,
        void *stream
    ) {
        if (nodeCount == 0) {
            return 0;
        }

        const size_t byteCount = (size_t)(nodeCount) * sizeof(zClass_NodeFreeListSlot);
        if (g_zClass_NodeArray == 0) {
            g_zClass_NodeArray = (zClass_NodeFreeListSlot *)(malloc(byteCount));
            g_zClass_NodeArraySize = nodeCount;
        } else if (nodeCount > g_zClass_NodeArraySize) {
            const int oldNodeCount = g_zClass_NodeArraySize;
            g_zClass_NodeArray =
                (zClass_NodeFreeListSlot *)(realloc(
                    g_zClass_NodeArray,
                    byteCount
                ));
            memset(
                &g_zClass_NodeArray[oldNodeCount],
                0,
                (size_t)(nodeCount - oldNodeCount) * sizeof(zClass_NodeFreeListSlot)
            );
            g_zClass_NodeArraySize = nodeCount;
        }

        if (fread(
            g_zClass_NodeArray,
            byteCount,
            1,
            (FILE *)(stream)
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x4a9,
                g_zClass_ReadGameZNodeBufferErrorMsg
            );
            return -1;
        }

        g_zClass_ActiveNodeCount = 0;
        for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
            zClass_NodePartial *node = &g_zClass_NodeArray[i].node;
            node->userDataOrDiRef =
                (unsigned int)((unsigned int)(zDi::IndexToPtrOrNull((int)(node->userDataOrDiRef))));
            node->actionCallback = 0;

            if (ReadSingleNodeClassData(
                node,
                stream
            ) > 0) {
                ++g_zClass_ActiveNodeCount;
                g_zClass_NodeArray[i].freeTag |= 0x01000000u;
            } else {
                g_zClass_NodeArray[i].freeTag &= 0xfeffffffu;
            }
        }

        for (zClass_TypeListLink *link = zClass_TypeList::Head(0x0d); link != 0;
            link = link->next) {
            zClass_NodePartial *node = link->node;
            if (node == 0) {
                continue;
            }

            zClass_WorldDataPartial *data = (zClass_WorldDataPartial *)(node->classData);
            for (int i = 0; i < data->lightCount; ++i) {
                data->lightDataList[i] =
                    (zClass_LightDataPartial *)(data->lightNodes[i]->classData);
            }

            data = (zClass_WorldDataPartial *)(node->classData);
            for (int i_775 = 0; i_775 < data->soundCount; ++i_775) {
                data->soundDataList[i_775] =
                    (zClass_SoundDataPartial *)(data->soundNodes[i_775]->classData);
            }
        }

        return g_zClass_NodeArraySize;
    }

    /**
     * Reimplements 0x455730: GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local.
     * Evidence: BN name/source-file comment and caller shape open the current
     * ZBD path, then delegate display-instance replacement to 0x4557a0.
     * Purpose: reload display instances for a node subtree from the current ZBD.
     */
    RECOIL_NO_GS int __fastcall ReloadDisplayInstancesFromCurrentPath_Local(
        zClass_NodePartial * node,
        int recurseChildren
    ) {
        if (strlen(g_zClass_CurrentZbdPath) == 0) {
            return 1;
        }

        zClass_ZbdHeader header;
        FILE *const file = GameZ::OpenAndReadZBDHeader(
            g_zClass_CurrentZbdPath,
            &header
        );
        if (file == 0) {
            return 1;
        }

        const int result =
            ReloadDisplayInstancesRecursive_Local(
                file,
                &header,
                node,
                recurseChildren
            );
        fclose(file);
        return result;
    }

    /**
     * Reimplements 0x4557a0: GameZ_ZBD::ReloadDisplayInstancesRecursive_Local.
     * Evidence: BN name/source-file comment and recursive caller path seek to
     * the serialized node slot, load the DI entry, and optionally visit children.
     * Purpose: replace one node's display instance from a ZBD and recurse.
     */
    RECOIL_NO_GS int __fastcall ReloadDisplayInstancesRecursive_Local(
        void *stream,
        zClass_ZbdHeader *zbdHeader,
        zClass_NodePartial *node,
        int recurseChildren
    ) {
        const int nodeIndex = NodePtrToIndex(node);
        if (nodeIndex < 0) {
            return 1;
        }

        if (nodeIndex >= zbdHeader->nodeCount) {
            return 1;
        }

        FILE *const file = (FILE *)(stream);
        fseek(
            file,
            zbdHeader->nodeTableOffset + nodeIndex * (int)(sizeof(zClass_NodeFreeListSlot)),
            SEEK_SET
        );

        zClass_NodeFreeListSlot serializedNode;
        if (fread(
            &serializedNode,
            sizeof(serializedNode),
            1,
            file
        ) != 1) {
            zError::ReportOld(
                0x200,
                g_zClass_SourceFile_ClsZbdC,
                0x5b5,
                g_zClass_ReadGameZNodeBufferErrorMsg
            );
            return 1;
        }

        const int displayInstanceIndex = (int)(serializedNode.node.userDataOrDiRef);
        fseek(
            file,
            zbdHeader->model3dOffset,
            SEEK_SET
        );

        unsigned int oldDisplayInstanceValue;
        zClass_Class::gwNodeGetUserData(
            node,
            &oldDisplayInstanceValue
        );
        zClass_Class::gwNodeSetDisplayInstance(
            node,
            0
        );

        zDiPartial *const displayInstance =
            zModel_DiPool::ReadEntryByIndexFromStream(
                file,
                displayInstanceIndex
            );
        if (displayInstance == 0) {
            zClass_Class::gwNodeSetDisplayInstance(
                node,
                (zDiPartial *)((unsigned int)(oldDisplayInstanceValue))
            );
        } else {
            zClass_Class::gwNodeSetDisplayInstance(
                node,
                displayInstance
            );
            zDiPartial *const oldDisplayInstance =
                (zDiPartial *)((unsigned int)(oldDisplayInstanceValue));
            if (oldDisplayInstance != 0) {
                zModel_DiPool::FreeIfUnreferenced(oldDisplayInstance);
            }
        }

        if (recurseChildren != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                ReloadDisplayInstancesRecursive_Local(
                    file,
                    zbdHeader,
                    node->listB[i],
                    recurseChildren
                );
            }
        }

        return 0;
    }
}
