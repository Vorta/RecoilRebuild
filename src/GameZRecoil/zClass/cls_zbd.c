#include "zClass.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zModel/zModel.h"
#include "zDi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
zClass_NodePartial **g_GameZ_Zbd_NodeIndexScratch = 0;
int g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
}

namespace {
    const char *kClsZbdSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_zbd.c";

    const int kZClassNodeCamera = 1;
    const int kZClassNodeWorld = 2;
    const int kZClassNodeWindow = 3;
    const int kZClassNodeDisplay = 4;
    const int kZClassNodeObject3D = 5;
    const int kZClassNodeLod = 6;
    const int kZClassNodeLight = 9;
    const int kZClassNodeSound = 10;

    RECOIL_FORCEINLINE int ReportZbdWriteFailure(int sourceLine, const char *message) {
        zError::ReportOld(0x200, kClsZbdSourceFile, sourceLine, message);
        return -1;
    }

    RECOIL_FORCEINLINE int ReportZbdReadFailure(int sourceLine, const char *message) {
        zError::ReportOld(0x200, kClsZbdSourceFile, sourceLine, message);
        return -1;
    }

    RECOIL_FORCEINLINE bool WriteZbdBlob(const void *data, size_t byteCount, void *stream) {
        return fwrite(data, byteCount, 1, static_cast<FILE *>(stream)) == 1;
    }

    RECOIL_FORCEINLINE bool ReadZbdBlob(void *data, size_t byteCount, void *stream) {
        return fread(data, byteCount, 1, static_cast<FILE *>(stream)) == 1;
    }
}

namespace GameZ {
    // Reimplements 0x454a50: GameZ::WriteZBDFile
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL WriteZBDFile(const char *filename) {
        const size_t filenameLength = strlen(filename);
        if (filenameLength == 0) {
            return -1;
        }

        if (filenameLength < 0x2f) {
            memcpy(g_zClass_CurrentZbdPath, filename, filenameLength + 1);
        } else {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x272,
                              "zbd_filename length %d exceeds storage string size %d.",
                              static_cast<int>(filenameLength), 0x30);
        }

        FILE *const file = fopen(filename, "wb");
        if (file == 0) {
            return -1;
        }

        zClass_ZbdHeader header;
        header.magic = 0x02971222;
        header.version = 0x0f;
        if (fwrite(&header, sizeof(header), 1, file) != 1) {
            return ReportZbdWriteFailure(0x285, "Error writing GameZ header data.");
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

        fseek(file, 0, SEEK_SET);
        if (fwrite(&header, sizeof(header), 1, file) != 1) {
            return ReportZbdWriteFailure(0x2aa, "Error writing GameZ header data.");
        }

        fclose(file);
        return 0;
    }

    // Reimplements 0x455520: GameZ::ReadZBDFile
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL ReadZBDFile(const char *filename) {
        const size_t filenameLength = strlen(filename);
        if (filenameLength == 0) {
            return -1;
        }

        if (filenameLength < 0x2f) {
            memcpy(g_zClass_CurrentZbdPath, filename, filenameLength + 1);
        } else {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x551,
                              "zbd_filename length %d exceeds storage string size %d.",
                              static_cast<int>(filenameLength), 0x30);
        }

        zClass_ZbdHeader header;
        FILE *const file = OpenAndReadZBDHeader(filename, &header);
        if (file == 0) {
            return -1;
        }

        int sourceLine = 0;
        const char *message = 0;

        fseek(file, header.texDirOffset, SEEK_SET);
        if (zImage::ReadTextureDirectory(header.texDirArg, file) < 0) {
            sourceLine = 0x562;
            message = "Error reading GameZ texture data.";
            goto readError;
        }

        fseek(file, header.matlOffset, SEEK_SET);
        if (zModel_MatlBuffer::ReadGameZ(file) < 0) {
            sourceLine = 0x56e;
            message = "Error reading GameZ material data.";
            goto readError;
        }

        fseek(file, header.model3dOffset, SEEK_SET);
        if (zModel_DiPool::ReadFromStream(file) < 0) {
            sourceLine = 0x57a;
            message = "Error reading GameZ model3d data.";
            goto readError;
        }

        fseek(file, header.nodeTableOffset, SEEK_SET);
        if (GameZ_ZBD::ReadNodeTable(header.nodeCount, file) < 0) {
            sourceLine = 0x586;
            message = "Error reading GameZ node data.";
            goto readError;
        }

        g_zClass_NodeFreeHeadIndex = header.nodeFreeHead;
        fclose(file);
        return 0;

    readError:
        zError::ReportOld(0x200, kClsZbdSourceFile, sourceLine, message);
        fclose(file);
        return -1;
    }

    // Reimplements 0x4556a0: GameZ::OpenAndReadZBDHeader
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE FILE *RECOIL_FASTCALL OpenAndReadZBDHeader(const char *filename,
                                                                    zClass_ZbdHeader *outHeader) {
        FILE *file = fopen(filename, "rb");
        if (file == 0) {
            return 0;
        }

        int sourceLine = 0;
        const char *message = 0;
        if (fread(outHeader, sizeof(zClass_ZbdHeader), 1, file) != 1) {
            message = "Error reading GameZ header data.";
            sourceLine = 0x515;
        } else if (outHeader->magic != 0x02971222) {
            message = "Error reading GameZ header data; incompatible file type";
            sourceLine = 0x51e;
        } else if (outHeader->version != 0x0f) {
            message = "Error reading GameZ header data; incompatible file version";
            sourceLine = 0x527;
        } else {
            return file;
        }

        zError::ReportOld(0x200, kClsZbdSourceFile, sourceLine, message);
        fclose(file);
        return 0;
    }
}

namespace zClass {
    // Reimplements 0x4543a0: zClass::NodePtrToValidatedIndex
    RECOIL_NOINLINE int RECOIL_FASTCALL NodePtrToValidatedIndex(zClass_NodePartial *
                                                                         node) {
        const int index = GameZ_ZBD::NodePtrToIndex(node);
        if (index >= 0 && (g_zClass_NodeArray[index].freeTag & 0x01000000u) != 0) {
            return index;
        }

        return -1;
    }
}

namespace GameZ_ZBD {
    // Reimplements 0x454370: GameZ_ZBD::NodePtrToIndex
    RECOIL_NOINLINE int RECOIL_FASTCALL NodePtrToIndex(zClass_NodePartial * node) {
        if (node == 0) {
            return -1;
        }

        return static_cast<int>(reinterpret_cast<zClass_NodeFreeListSlot *>(node) -
                                         g_zClass_NodeArray);
    }

    // Reimplements 0x4543d0: GameZ_ZBD::NodeIndexToPtr
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL NodeIndexToPtr(int index) {
        if (index < 0) {
            return 0;
        }

        return &g_zClass_NodeArray[index].node;
    }

    // Reimplements 0x4543f0: GameZ_ZBD::WriteNodeRefListIndices
    RECOIL_NOINLINE int RECOIL_FASTCALL WriteNodeRefListIndices(
        zClass_NodePartial * *nodeRefList, int entryCount, void *stream) {
        if (entryCount == 0) {
            return 0;
        }

        const size_t byteCount = static_cast<size_t>(entryCount) * sizeof(unsigned int);
        if (entryCount > g_GameZ_Zbd_NodeIndexScratchCapacity) {
            g_GameZ_Zbd_NodeIndexScratch = static_cast<zClass_NodePartial **>(
                realloc(g_GameZ_Zbd_NodeIndexScratch, byteCount));
            g_GameZ_Zbd_NodeIndexScratchCapacity = entryCount;
        }

        memcpy(g_GameZ_Zbd_NodeIndexScratch, nodeRefList, byteCount);
        int *indices = reinterpret_cast<int *>(g_GameZ_Zbd_NodeIndexScratch);
        for (int i = 0; i < entryCount; ++i) {
            indices[i] = NodePtrToIndex(g_GameZ_Zbd_NodeIndexScratch[i]);
        }

        if (fwrite(g_GameZ_Zbd_NodeIndexScratch, byteCount, 1,
                        static_cast<FILE *>(stream)) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0xd7, "Error writing node data.");
            return -1;
        }

        return 0;
    }

    // Reimplements 0x4544b0: GameZ_ZBD::WriteSingleNodeClassData
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL WriteSingleNodeClassData(
        zClass_NodePartial * node, void *stream) {
        int result = 0;
        if (node->actionCallback != 0) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0xf1,
                              "Writing gamez.zbd; node %s has action callback defined.",
                              node->name);
        }

        switch (node->classId) {
        case 0:
            break;

        case kZClassNodeSound: {
            result = 1;
            zError::ReportOld(0x200, kClsZbdSourceFile, 0xfc,
                              "Writing sound node data: Must complete software.");

            zClass_SoundDataPartial *data = static_cast<zClass_SoundDataPartial *>(node->classData);
            if (!WriteZbdBlob(data, sizeof(zClass_SoundDataPartial), stream)) {
                return ReportZbdWriteFailure(0x105, "Error writing node sound data.");
            }

            if (data->attachedWorldCount > 0) {
                WriteNodeRefListIndices(data->attachedWorlds, data->attachedWorldCount, stream);
            }
            break;
        }

        case kZClassNodeObject3D:
            result = 1;
            if (!WriteZbdBlob(node->classData, sizeof(zClass_Object3DDataPartial), stream)) {
                return ReportZbdWriteFailure(0x119, "Error writing node object3d data.");
            }
            break;

        case kZClassNodeLod:
            result = 1;
            if (!WriteZbdBlob(node->classData, sizeof(zClass_LodDataPartial), stream)) {
                return ReportZbdWriteFailure(0x128, "Error writing node lod data.");
            }
            break;

        case kZClassNodeLight: {
            result = 1;
            zClass_LightDataPartial *data = static_cast<zClass_LightDataPartial *>(node->classData);
            if (!WriteZbdBlob(data, sizeof(zClass_LightDataPartial), stream)) {
                return ReportZbdWriteFailure(0x137, "Error writing node light data.");
            }

            if (data->attachedWorldCount > 0) {
                WriteNodeRefListIndices(data->attachedWorlds, data->attachedWorldCount, stream);
            }
            break;
        }

        case kZClassNodeCamera: {
            result = 1;
            zClass_CameraDataPartial data;
            memcpy(&data, node->classData, sizeof(data));
            data.worldNode = reinterpret_cast<zClass_NodePartial *>(
                static_cast<int>(NodePtrToIndex(data.worldNode)));
            data.windowNode = reinterpret_cast<zClass_NodePartial *>(
                static_cast<int>(NodePtrToIndex(data.windowNode)));
            data.horizonNode = reinterpret_cast<zClass_NodePartial *>(
                static_cast<int>(NodePtrToIndex(data.horizonNode)));
            data.horizonXZNode = reinterpret_cast<zClass_NodePartial *>(
                static_cast<int>(NodePtrToIndex(data.horizonXZNode)));

            if (!WriteZbdBlob(&data, sizeof(data), stream)) {
                return ReportZbdWriteFailure(0x15a, "Error writing node camera data.");
            }
            break;
        }

        case kZClassNodeDisplay:
            result = 1;
            if (!WriteZbdBlob(node->classData, sizeof(zClass_DisplayDataPartial), stream)) {
                return ReportZbdWriteFailure(0x16a, "Error writing node display data.");
            }
            break;

        case kZClassNodeWindow:
            result = 1;
            if (!WriteZbdBlob(node->classData, sizeof(zClass_WindowDataPartial), stream)) {
                return ReportZbdWriteFailure(0x179, "Error writing node window data.");
            }
            break;

        case kZClassNodeWorld: {
            result = 1;
            zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(node->classData);
            if (!WriteZbdBlob(data, sizeof(zClass_WorldDataPartial), stream)) {
                return ReportZbdWriteFailure(0x18c, "Error writing node world data.");
            }

            if (data->lightCount > 0) {
                WriteNodeRefListIndices(data->lightNodes, data->lightCount, stream);
            }
            if (data->soundCount > 0) {
                WriteNodeRefListIndices(data->soundNodes, data->soundCount, stream);
            }

            {
            for (int row = 0; row < data->areaGridRowCount; ++row) {
                zWorldAreaPartial *area = data->areaGridRows[row];
                {
                for (int col = 0; col < data->areaGridColCount; ++col) {
                    if (!WriteZbdBlob(area, sizeof(zWorldAreaPartial), stream)) {
                        return ReportZbdWriteFailure(0x1a8,
                                                     "Error writing world area partition data.");
                    }

                    if (area->childCount > 0) {
                        WriteNodeRefListIndices(area->childList, area->childCount, stream);
                    }
                    ++area;
                }
                }
            }
            }
            break;
        }

        default:
            zError::ReportOld(0x400, kClsZbdSourceFile, 0x1bd,
                              "gClsWriteNode(): Unrecognized node class type:\n"
                              "  node = %s class_type = %d\n",
                              node->name, node->classId);
            return 0;
        }

        if (node->listCountA > 0) {
            result = 1;
            WriteNodeRefListIndices(node->listA, node->listCountA, stream);
        }
        if (node->listCountB > 0) {
            result = 1;
            WriteNodeRefListIndices(node->listB, node->listCountB, stream);
        }

        return result;
    }

    // Reimplements 0x454890: GameZ_ZBD::WriteNodeTable
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL WriteNodeTable(void *stream) {
        int result = g_zClass_NodeArraySize;
        if (result == 0) {
            return result;
        }

        const int byteCount =
            result * static_cast<int>(sizeof(zClass_NodeFreeListSlot));
        zClass_NodeFreeListSlot *nodeBuffer =
            static_cast<zClass_NodeFreeListSlot *>(malloc(byteCount));
        memcpy(nodeBuffer, g_zClass_NodeArray, byteCount);

        for (int i = 0; i < result; ++i) {
            zClass_NodePartial *node = &nodeBuffer[i].node;
            if (node->listCountA == 0) {
                node->listA = 0;
            }
            if (node->listCountB == 0) {
                node->listB = 0;
            }
            node->userDataOrDiRef =
                static_cast<unsigned int>(zDi::PtrToIndexOrMinus1(reinterpret_cast<zDiPartial *>(
                    static_cast<unsigned int>(node->userDataOrDiRef))));
            node->actionCallback = 0;
        }

        FILE *file = static_cast<FILE *>(stream);
        const long nodeTableOffset = ftell(file);
        if (fwrite(nodeBuffer, byteCount, 1, file) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x218, "Error writing node data.");
            result = 0;
        }

        if (result > 0) {
            for (int i = 0; i < result; ++i) {
                const long classDataOffset = ftell(file);
                if (WriteSingleNodeClassData(&nodeBuffer[i].node, stream) != 0) {
                    const unsigned int freeTag = nodeBuffer[i].freeTag;
                    nodeBuffer[i].freeTag =
                        ((static_cast<unsigned int>(classDataOffset) ^ freeTag) & 0x00ffffffu) ^
                        freeTag;
                }
            }
        }

        const long endOffset = ftell(file);
        fseek(file, nodeTableOffset, SEEK_SET);
        if (fwrite(nodeBuffer, g_zClass_NodeArraySize * sizeof(zClass_NodeFreeListSlot), 1,
                        file) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x23a, "Error writing node data.");
            result = 0;
        }
        fseek(file, endOffset, SEEK_SET);

        if (g_GameZ_Zbd_NodeIndexScratch != 0) {
            free(g_GameZ_Zbd_NodeIndexScratch);
            g_GameZ_Zbd_NodeIndexScratch = 0;
        }
        g_GameZ_Zbd_NodeIndexScratchCapacity = 0;

        free(nodeBuffer);
        return result;
    }

    // Reimplements 0x454bf0: GameZ_ZBD::ReadNodeRefListIndices
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadNodeRefListIndices(
        zClass_NodePartial * *nodeRefList, int entryCount, void *stream) {
        if (entryCount == 0) {
            return 0;
        }

        const size_t byteCount = static_cast<size_t>(entryCount) * sizeof(unsigned int);
        if (fread(nodeRefList, byteCount, 1, static_cast<FILE *>(stream)) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x2d0, "Error reading GameZ Node list.");
            return -1;
        }

        for (int i = 0; i < entryCount; ++i) {
            const int index =
                static_cast<int>(reinterpret_cast<int>(nodeRefList[i]));
            nodeRefList[i] = NodeIndexToPtr(index);
        }

        return 0;
    }

    // Reimplements 0x454c60: GameZ_ZBD::ReadSingleNodeClassData
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadSingleNodeClassData(zClass_NodePartial * node,
                                                                         void *stream) {
        int result = 0;
        switch (node->classId) {
        case 0:
            break;

        case kZClassNodeSound: {
            result = 1;
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x2f1,
                              "Reading sound node data: Must complete software.");

            zClass_SoundDataPartial *data = static_cast<zClass_SoundDataPartial *>(
                malloc(sizeof(zClass_SoundDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(data, sizeof(zClass_SoundDataPartial), stream)) {
                return ReportZbdReadFailure(0x2fc, "Error reading node sound data.");
            }

            data->sample = 0;
            data->playHandle = 0;
            if (data->attachedWorldCount > 0) {
                data->attachedWorlds = static_cast<zClass_NodePartial **>(
                    malloc(data->attachedWorldCount * sizeof(zClass_NodePartial *)));
                ReadNodeRefListIndices(data->attachedWorlds, data->attachedWorldCount, stream);
            } else {
                data->attachedWorlds = 0;
            }

            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(0x0a, node);
            break;
        }

        case kZClassNodeObject3D:
            result = 1;
            node->classData = malloc(sizeof(zClass_Object3DDataPartial));
            if (!ReadZbdBlob(node->classData, sizeof(zClass_Object3DDataPartial), stream)) {
                return ReportZbdReadFailure(0x323, "Error reading node object3d data.");
            }
            zClass_TypeList::Insert(6, node);
            break;

        case kZClassNodeLod:
            result = 1;
            node->classData = malloc(sizeof(zClass_LodDataPartial));
            if (!ReadZbdBlob(node->classData, sizeof(zClass_LodDataPartial), stream)) {
                return ReportZbdReadFailure(0x338, "Error reading node lod data.");
            }
            zClass_TypeList::Insert(6, node);
            break;

        case kZClassNodeLight: {
            result = 1;
            zClass_LightDataPartial *data = static_cast<zClass_LightDataPartial *>(
                malloc(sizeof(zClass_LightDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(data, sizeof(zClass_LightDataPartial), stream)) {
                return ReportZbdReadFailure(0x34d, "Error reading node light data.");
            }

            if (data->attachedWorldCount > 0) {
                data->attachedWorlds = static_cast<zClass_NodePartial **>(
                    malloc(data->attachedWorldCount * sizeof(zClass_NodePartial *)));
                ReadNodeRefListIndices(data->attachedWorlds, data->attachedWorldCount, stream);
            } else {
                data->attachedWorlds = 0;
            }

            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(9, node);
            break;
        }

        case kZClassNodeCamera: {
            result = 1;
            zClass_CameraDataPartial *data = static_cast<zClass_CameraDataPartial *>(
                malloc(sizeof(zClass_CameraDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(data, sizeof(zClass_CameraDataPartial), stream)) {
                return ReportZbdReadFailure(0x371, "Error reading node camera data.");
            }

            data->worldNode = NodeIndexToPtr(
                static_cast<int>(reinterpret_cast<int>(data->worldNode)));
            data->windowNode = NodeIndexToPtr(
                static_cast<int>(reinterpret_cast<int>(data->windowNode)));
            data->horizonNode = NodeIndexToPtr(
                static_cast<int>(reinterpret_cast<int>(data->horizonNode)));
            data->horizonXZNode = NodeIndexToPtr(
                static_cast<int>(reinterpret_cast<int>(data->horizonXZNode)));

            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(8, node);
            zClass_Camera::gwCameraSetNearFarClip(node, data->nearClip, data->farClip);
            zClass_Camera::gwCameraSetViewport(node, data->viewportWidth, data->viewportHeight);
            break;
        }

        case kZClassNodeDisplay: {
            result = 1;
            zClass_DisplayDataPartial *data = static_cast<zClass_DisplayDataPartial *>(
                malloc(sizeof(zClass_DisplayDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(data, sizeof(zClass_DisplayDataPartial), stream)) {
                return ReportZbdReadFailure(0x39a, "Error reading node display data.");
            }

            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(0x0f, node);
            zClass_Display::gwDisplaySetBackgroundColor(node, data->backgroundR, data->backgroundG,
                                                        data->backgroundB);
            break;
        }

        case kZClassNodeWindow:
            result = 1;
            node->classData = malloc(sizeof(zClass_WindowDataPartial));
            if (!ReadZbdBlob(node->classData, sizeof(zClass_WindowDataPartial), stream)) {
                return ReportZbdReadFailure(0x3b7, "Error reading node window data.");
            }
            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(0x0e, node);
            break;

        case kZClassNodeWorld: {
            result = 1;
            zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(
                malloc(sizeof(zClass_WorldDataPartial)));
            node->classData = data;
            if (!ReadZbdBlob(data, sizeof(zClass_WorldDataPartial), stream)) {
                return ReportZbdReadFailure(0x3d4, "Error reading node world data.");
            }

            if (data->lightCount > 0) {
                data->lightNodes = static_cast<zClass_NodePartial **>(
                    malloc(data->lightCount * sizeof(zClass_NodePartial *)));
                ReadNodeRefListIndices(data->lightNodes, data->lightCount, stream);
                data->lightDataList = static_cast<zClass_LightDataPartial **>(
                    malloc(data->lightCount * sizeof(zClass_LightDataPartial *)));
            } else {
                data->lightNodes = 0;
                data->lightDataList = 0;
            }

            if (data->soundCount > 0) {
                data->soundNodes = static_cast<zClass_NodePartial **>(
                    malloc(data->soundCount * sizeof(zClass_NodePartial *)));
                ReadNodeRefListIndices(data->soundNodes, data->soundCount, stream);
                data->soundDataList = static_cast<zClass_SoundDataPartial **>(
                    malloc(data->soundCount * sizeof(zClass_SoundDataPartial *)));
            } else {
                data->soundNodes = 0;
                data->soundDataList = 0;
            }

            data->areaGridRows = static_cast<zWorldAreaPartial **>(
                calloc(data->areaGridRowCount, sizeof(zWorldAreaPartial *)));
            {
            for (int row = 0; row < data->areaGridRowCount; ++row) {
                data->areaGridRows[row] = static_cast<zWorldAreaPartial *>(
                    calloc(data->areaGridColCount, sizeof(zWorldAreaPartial)));
            }
            }

            {
            for (int row = 0; row < data->areaGridRowCount; ++row) {
                zWorldAreaPartial *area = data->areaGridRows[row];
                {
                for (int col = 0; col < data->areaGridColCount; ++col) {
                    if (!ReadZbdBlob(area, sizeof(zWorldAreaPartial), stream)) {
                        return ReportZbdReadFailure(0x423,
                                                    "Error reading world area partition data.");
                    }

                    if (area->childCount > 0) {
                        area->childList = static_cast<zClass_NodePartial **>(
                            malloc(area->childCount * sizeof(zClass_NodePartial *)));
                        ReadNodeRefListIndices(area->childList, area->childCount, stream);
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
            zClass_TypeList::Insert(6, node);
            zClass_TypeList::Insert(0x0d, node);
            zClass_World::SetPendingFogState(node, data->fogState);
            zClass_World::SetPendingFogColorRgb01(
                node, data->ambientColor.red, data->ambientColor.green, data->ambientColor.blue);
            zClass_World::SetPendingFogAltitudeRange(node, data->fogHeightLow, data->fogHeightHigh);
            zClass_World::SetPendingFogRange(node, data->fogDistanceStart, data->fogDistanceEnd);
            zClass_World::SetPendingFogDensity(node, data->fogDensity);
            zClass_World::ApplyPendingFogSettings(node);
            break;
        }

        default:
            zError::ReportOld(0x400, kClsZbdSourceFile, 0x45d,
                              "gClsReadNode(): Unrecognized node class type:\n"
                              "  node = %s class_type = %d\n",
                              node->name, node->classId);
            return -1;
        }

        if (node->listCountA > 0) {
            result = 1;
            node->listA = static_cast<zClass_NodePartial **>(
                malloc(node->listCountA * sizeof(zClass_NodePartial *)));
            ReadNodeRefListIndices(node->listA, node->listCountA, stream);
        } else {
            node->listA = 0;
        }

        if (node->listCountB > 0) {
            node->listB = static_cast<zClass_NodePartial **>(
                malloc(node->listCountB * sizeof(zClass_NodePartial *)));
            ReadNodeRefListIndices(node->listB, node->listCountB, stream);
            return 1;
        }

        node->listB = 0;
        return result;
    }

    // Reimplements 0x455350: GameZ_ZBD::ReadNodeTable
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE int RECOIL_FASTCALL ReadNodeTable(int nodeCount,
                                                               void *stream) {
        if (nodeCount == 0) {
            return 0;
        }

        const size_t byteCount =
            static_cast<size_t>(nodeCount) * sizeof(zClass_NodeFreeListSlot);
        if (g_zClass_NodeArray == 0) {
            g_zClass_NodeArray = static_cast<zClass_NodeFreeListSlot *>(malloc(byteCount));
            g_zClass_NodeArraySize = nodeCount;
        } else if (nodeCount > g_zClass_NodeArraySize) {
            const int oldNodeCount = g_zClass_NodeArraySize;
            g_zClass_NodeArray =
                static_cast<zClass_NodeFreeListSlot *>(realloc(g_zClass_NodeArray, byteCount));
            memset(&g_zClass_NodeArray[oldNodeCount], 0,
                        static_cast<size_t>(nodeCount - oldNodeCount) *
                            sizeof(zClass_NodeFreeListSlot));
            g_zClass_NodeArraySize = nodeCount;
        }

        if (fread(g_zClass_NodeArray, byteCount, 1, static_cast<FILE *>(stream)) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x4a9, "Error reading GameZ Node buffer.");
            return -1;
        }

        g_zClass_ActiveNodeCount = 0;
        for (int i = 0; i < g_zClass_NodeArraySize; ++i) {
            zClass_NodePartial *node = &g_zClass_NodeArray[i].node;
            node->userDataOrDiRef = static_cast<unsigned int>(reinterpret_cast<unsigned int>(
                zDi::IndexToPtrOrNull(static_cast<int>(node->userDataOrDiRef))));
            node->actionCallback = 0;

            if (ReadSingleNodeClassData(node, stream) > 0) {
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

            zClass_WorldDataPartial *data = static_cast<zClass_WorldDataPartial *>(node->classData);
            for (int i = 0; i < data->lightCount; ++i) {
                data->lightDataList[i] =
                    static_cast<zClass_LightDataPartial *>(data->lightNodes[i]->classData);
            }

            data = static_cast<zClass_WorldDataPartial *>(node->classData);
            for (int i_775 = 0; i_775 < data->soundCount; ++i_775) {
                data->soundDataList[i_775] =
                    static_cast<zClass_SoundDataPartial *>(data->soundNodes[i_775]->classData);
            }
        }

        return g_zClass_NodeArraySize;
    }

    // Reimplements 0x455730:
    // GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL
    ReloadDisplayInstancesFromCurrentPath_Local(zClass_NodePartial * node,
                                                int recurseChildren) {
        if (strlen(g_zClass_CurrentZbdPath) == 0) {
            return 1;
        }

        zClass_ZbdHeader header;
        FILE *const file = GameZ::OpenAndReadZBDHeader(g_zClass_CurrentZbdPath, &header);
        if (file == 0) {
            return 1;
        }

        const int result =
            ReloadDisplayInstancesRecursive_Local(file, &header, node, recurseChildren);
        fclose(file);
        return result;
    }

    // Reimplements 0x4557a0:
    // GameZ_ZBD::ReloadDisplayInstancesRecursive_Local
    // (D:\Proj\GameZRecoil\zClass\cls_zbd.c)
    RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL ReloadDisplayInstancesRecursive_Local(
        void *stream, zClass_ZbdHeader *zbdHeader, zClass_NodePartial *node,
        int recurseChildren) {
        const int nodeIndex = NodePtrToIndex(node);
        if (nodeIndex < 0) {
            return 1;
        }

        if (nodeIndex >= zbdHeader->nodeCount) {
            return 1;
        }

        FILE *const file = static_cast<FILE *>(stream);
        fseek(file,
                   zbdHeader->nodeTableOffset +
                       nodeIndex * static_cast<int>(sizeof(zClass_NodeFreeListSlot)),
                   SEEK_SET);

        zClass_NodeFreeListSlot serializedNode;
        if (fread(&serializedNode, sizeof(serializedNode), 1, file) != 1) {
            zError::ReportOld(0x200, kClsZbdSourceFile, 0x5b5, "Error reading GameZ Node buffer.");
            return 1;
        }

        const int displayInstanceIndex =
            static_cast<int>(serializedNode.node.userDataOrDiRef);
        fseek(file, zbdHeader->model3dOffset, SEEK_SET);

        unsigned int oldDisplayInstanceValue;
        zClass_Class::gwNodeGetUserData(node, &oldDisplayInstanceValue);
        zClass_Class::gwNodeSetDisplayInstance(node, 0);

        zDiPartial *const displayInstance =
            zModel_DiPool::ReadEntryByIndexFromStream(file, displayInstanceIndex);
        if (displayInstance == 0) {
            zClass_Class::gwNodeSetDisplayInstance(
                node, reinterpret_cast<zDiPartial *>(
                          static_cast<unsigned int>(oldDisplayInstanceValue)));
        } else {
            zClass_Class::gwNodeSetDisplayInstance(node, displayInstance);
            zDiPartial *const oldDisplayInstance = reinterpret_cast<zDiPartial *>(
                static_cast<unsigned int>(oldDisplayInstanceValue));
            if (oldDisplayInstance != 0) {
                zModel_DiPool::FreeIfUnreferenced(oldDisplayInstance);
            }
        }

        if (recurseChildren != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                ReloadDisplayInstancesRecursive_Local(file, zbdHeader, node->listB[i],
                                                      recurseChildren);
            }
        }

        return 0;
    }
}
