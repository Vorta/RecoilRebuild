#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "zClass.h"

#include <windows.h>

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
void WriteU32(HANDLE file, std::uint32_t value) {
    DWORD written = 0;
    WriteFile(file, &value, sizeof(value), &written, nullptr);
}

void WriteBytes(HANDLE file, const char *text, std::uint32_t length) {
    DWORD written = 0;
    WriteFile(file, text, length, &written, nullptr);
}

void EnsureZrdrFreePool() {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }
}
} // namespace

extern "C" int zreader_named_int_lookup_smoke(void) {
    zReader::Node nestedChildren[4] = {};
    nestedChildren[0].type = zReader::ZRDR_NODE_INT;
    nestedChildren[0].value.i32 = 4;
    nestedChildren[1].type = zReader::ZRDR_NODE_STRING;
    nestedChildren[1].value.str = const_cast<char *>("nestedValue");
    nestedChildren[2].type = zReader::ZRDR_NODE_INT;
    nestedChildren[2].value.i32 = 77;
    nestedChildren[3].type = zReader::ZRDR_NODE_FLOAT;

    zReader::Node childArray[3] = {};
    childArray[0].type = zReader::ZRDR_NODE_INT;
    childArray[0].value.i32 = 3;
    childArray[1].type = zReader::ZRDR_NODE_INT;
    childArray[1].value.i32 = 1234;

    zReader::Node rootChildren[6] = {};
    rootChildren[0].type = zReader::ZRDR_NODE_INT;
    rootChildren[0].value.i32 = 6;
    rootChildren[1].type = zReader::ZRDR_NODE_STRING;
    rootChildren[1].value.str = const_cast<char *>("directValue");
    rootChildren[2].type = zReader::ZRDR_NODE_INT;
    rootChildren[2].value.i32 = 42;
    rootChildren[3].type = zReader::ZRDR_NODE_STRING;
    rootChildren[3].value.str = const_cast<char *>("arrayValue");
    rootChildren[4].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[4].value.nodes = childArray;
    rootChildren[5].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[5].value.nodes = nestedChildren;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootChildren;

    std::int32_t value = 0;
    if (zReader::ReadNamedInt(&root, "directValue", &value) != 1 || value != 42) {
        return 1;
    }

    value = 0;
    if (zReader::ReadNamedInt(&root, "arrayValue", &value) != 1 || value != 1234) {
        return 2;
    }

    value = 0;
    if (zReader::ReadNamedInt(&root, "nestedValue", &value) != 1 || value != 77) {
        return 3;
    }

    return zReader::ReadNamedInt(&root, "missingValue", &value) == 0 ? 0 : 4;
}

extern "C" int zreader_named_string_float_lookup_smoke(void) {
    zReader::Node stringArray[2] = {};
    stringArray[0].type = zReader::ZRDR_NODE_INT;
    stringArray[0].value.i32 = 2;
    stringArray[1].type = zReader::ZRDR_NODE_STRING;
    stringArray[1].value.str = const_cast<char *>("wrapped");

    zReader::Node floatArray[2] = {};
    floatArray[0].type = zReader::ZRDR_NODE_INT;
    floatArray[0].value.i32 = 2;
    floatArray[1].type = zReader::ZRDR_NODE_INT;
    floatArray[1].value.i32 = 9;

    zReader::Node rootChildren[9] = {};
    rootChildren[0].type = zReader::ZRDR_NODE_INT;
    rootChildren[0].value.i32 = 9;
    rootChildren[1].type = zReader::ZRDR_NODE_STRING;
    rootChildren[1].value.str = const_cast<char *>("directString");
    rootChildren[2].type = zReader::ZRDR_NODE_STRING;
    rootChildren[2].value.str = const_cast<char *>("plain");
    rootChildren[3].type = zReader::ZRDR_NODE_STRING;
    rootChildren[3].value.str = const_cast<char *>("arrayString");
    rootChildren[4].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[4].value.nodes = stringArray;
    rootChildren[5].type = zReader::ZRDR_NODE_STRING;
    rootChildren[5].value.str = const_cast<char *>("directFloat");
    rootChildren[6].type = zReader::ZRDR_NODE_FLOAT;
    rootChildren[6].value.f32 = 1.5f;
    rootChildren[7].type = zReader::ZRDR_NODE_STRING;
    rootChildren[7].value.str = const_cast<char *>("arrayFloat");
    rootChildren[8].type = zReader::ZRDR_NODE_ARRAY;
    rootChildren[8].value.nodes = floatArray;

    zReader::Node root = {};
    root.type = zReader::ZRDR_NODE_ARRAY;
    root.value.nodes = rootChildren;

    if (std::strcmp(zReader::ReadNamedString(&root, "directString"), "plain") != 0 ||
        std::strcmp(zReader::ReadNamedString(&root, "arrayString"), "wrapped") != 0 ||
        zReader::ReadNamedString(&root, "missingString") != nullptr) {
        return 1;
    }

    float value = 0.0f;
    if (zReader::ReadNamedFloat(&root, "directFloat", &value) != 1 || value != 1.5f) {
        return 2;
    }

    value = 0.0f;
    if (zReader::ReadNamedFloat(&root, "arrayFloat", &value) != 1 || value != 9.0f) {
        return 3;
    }

    return zReader::ReadNamedFloat(&root, "missingFloat", &value) == 0 ? 0 : 4;
}

extern "C" int zreader_global_string_prefix_index_smoke(void) {
    char *savedTable[8] = {};
    for (int index = 0; index < 8; ++index) {
        savedTable[index] = g_zRndr_GlobalStringTable[index];
    }
    const int savedCount = g_zRndr_GlobalStringCount;

    g_zRndr_GlobalStringTable[0] = const_cast<char *>("ALPHA");
    g_zRndr_GlobalStringTable[1] = const_cast<char *>("BETA");
    g_zRndr_GlobalStringTable[2] = const_cast<char *>("LONG");
    g_zRndr_GlobalStringTable[3] = const_cast<char *>("door");
    g_zRndr_GlobalStringCount = 4;

    int result = 0;
    if (zReader::FindGlobalStringPrefixIndex(nullptr) != -1) {
        result = 1;
    }
    else if (zReader::FindGlobalStringPrefixIndex("ALPHA") != 0) {
        result = 2;
    }
    else if (zReader::FindGlobalStringPrefixIndex("alpha value") != 0) {
        result = 3;
    }
    else if (zReader::FindGlobalStringPrefixIndex("BETA\tvalue") != 1) {
        result = 4;
    }
    else if (zReader::FindGlobalStringPrefixIndex("alphabet") != -1) {
        result = 5;
    }
    else if (zReader::FindGlobalStringPrefixIndex("LON") != -1) {
        result = 6;
    }
    else if (zReader::FindGlobalStringPrefixIndex("DOOR") != 3) {
        result = 7;
    }
    else if (zReader::FindGlobalStringPrefixIndex("door-frame") != -1) {
        result = 8;
    }

    g_zRndr_GlobalStringCount = 0;
    if (result == 0 && zReader::FindGlobalStringPrefixIndex("ALPHA") != -1) {
        result = 9;
    }

    g_zRndr_GlobalStringCount = savedCount;
    for (int index = 0; index < 8; ++index) {
        g_zRndr_GlobalStringTable[index] = savedTable[index];
    }

    return result;
}

extern "C" int zrndr_global_string_table_load_dynamic_entries_smoke(void) {
    char *savedTable[100] = {};
    for (int index = 0; index < 100; ++index) {
        savedTable[index] = g_zRndr_GlobalStringTable[index];
    }
    const int savedCount = g_zRndr_GlobalStringCount;
    zArchiveList *const savedMountedList = g_zArchive_MountedList;

    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zgs", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 2);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 6);

    const char *strings[] = {"alpha child", "gamma", "delta", "delta more", "epsilon"};
    for (int index = 0; index < 5; ++index) {
        WriteU32(file, zReader::ZRDR_NODE_STRING);
        WriteU32(file, static_cast<std::uint32_t>(std::strlen(strings[index])));
        WriteBytes(file, strings[index], static_cast<std::uint32_t>(std::strlen(strings[index])));
    }
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "globals.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;
    zArchiveList mountedList = {};
    mountedList.count = 1;
    mountedList.head = &archiveNode;
    g_zArchive_MountedList = &mountedList;

    g_zRndr_GlobalStringTable[0] = const_cast<char *>("ALPHA");
    g_zRndr_GlobalStringTable[1] = const_cast<char *>("BETA");
    g_zRndr_GlobalStringTable[2] = const_cast<char *>("BASE2");
    g_zRndr_GlobalStringTable[3] = const_cast<char *>("BASE3");
    g_zRndr_GlobalStringTable[4] = const_cast<char *>("BASE4");
    g_zRndr_GlobalStringTable[5] = const_cast<char *>("BASE5");
    g_zRndr_GlobalStringCount = 6;

    char zrdPath[] = "globals.zrd";
    zRndr_GlobalStringTable::LoadDynamicEntriesFromPath(zrdPath);

    const bool ok = g_zRndr_GlobalStringCount == 9 &&
                    std::strcmp(g_zRndr_GlobalStringTable[6], "gamma") == 0 &&
                    std::strcmp(g_zRndr_GlobalStringTable[7], "delta") == 0 &&
                    std::strcmp(g_zRndr_GlobalStringTable[8], "epsilon") == 0;

    for (int index = 6; index < g_zRndr_GlobalStringCount; ++index) {
        std::free(g_zRndr_GlobalStringTable[index]);
    }
    for (int index = 0; index < 100; ++index) {
        g_zRndr_GlobalStringTable[index] = savedTable[index];
    }
    g_zRndr_GlobalStringCount = savedCount;
    g_zArchive_MountedList = savedMountedList;

    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 3;
}

extern "C" int zreader_load_node_from_archive_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zrd", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                              CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    WriteU32(file, 0xaaaaaaaa);
    WriteU32(file, 0xbbbbbbbb);
    const std::uint32_t zrdOffset = SetFilePointer(file, 0, nullptr, FILE_CURRENT);

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 3);
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, 6);
    WriteBytes(file, "SYNTAX", 6);
    WriteU32(file, zReader::ZRDR_NODE_INT);
    WriteU32(file, 2);
    FlushFileBuffers(file);

    zZarFileRecord record = {};
    record.fileOffset = zrdOffset;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT) - zrdOffset;
    std::strcpy(record.name, "sound.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode node = {};
    node.payload = &archive;
    node.next = &node;
    node.prev = &node;

    zArchiveList list = {};
    list.count = 1;
    list.head = &node;
    g_zArchive_MountedList = &list;

    std::uint32_t openedSize = 0;
    if (archive.FindRecordByNameCI("SOUND.ZRD") != &record ||
        archive.OpenFileByName("sound.zrd", &openedSize) != file || openedSize != record.fileSize ||
        zArchiveList_GetCount(&list) != 1 || zArchiveList_GetAt(&list, 0) != &archive) {
        CloseHandle(file);
        DeleteFileA(tempPath);
        return 3;
    }

    zReader::Node *root = zReader::LoadNodeFromPath("C:\\dummy\\sound.zrd");
    std::int32_t syntax = 0;
    const bool ok =
        root != nullptr && zReader::ReadNamedInt(root, "SYNTAX", &syntax) == 1 && syntax == 2;

    if (root != nullptr) {
        std::free(root->value.nodes[1].value.str);
        std::free(root->value.nodes);
        std::free(root);
    }

    g_zArchive_MountedList = nullptr;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 4;
}

extern "C" int zreader_file_exists_and_list_create_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zrf", 0, tempPath) == 0) {
        return 1;
    }

    zArchiveList *list = zArchiveList_CreateEmpty();
    const bool ok = list != nullptr && list->count == 0 && list->head == nullptr &&
                    zReader::FileExists(tempPath) == 1 && zReader_FileExists_Wrapper(tempPath) == 1;

    std::free(list);
    DeleteFileA(tempPath);
    return ok && zReader::FileExists(tempPath) == 0 ? 0 : 2;
}

extern "C" int zreader_archive_list_and_search_paths_smoke(void) {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zra", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zrb", 0, tempPathB) == 0) {
        return 1;
    }

    zArchiveList *list = zArchiveList_CreateEmpty();
    char searchText[MAX_PATH * 3] = {};
    std::strcpy(searchText, tempPathA);
    std::strcat(searchText, ";");
    std::strcat(searchText, "C:\\missing\\not_here.zrd");
    std::strcat(searchText, ";");
    std::strcat(searchText, tempPathB);
    std::strcat(searchText, ";");
    std::strcat(searchText, tempPathA);

    zUtil::ZRDR_AddSearchPaths(list, searchText);

    const bool listOk =
        list->count == 2 &&
        std::strcmp(static_cast<const char *>(list->head->payload), tempPathB) == 0 &&
        std::strcmp(static_cast<const char *>(list->head->next->payload), tempPathA) == 0 &&
        zArchiveList_GetCount(list) == 2 && zArchiveList_GetAt(list, 0) == list->head->payload &&
        zArchiveList_FindPayloadByPredicate_Thunk(list, zUtil_ZRDR_StrCmpPredicate, tempPathA) ==
            list->head->next->payload &&
        zUtil_ZRDR_StrCmpPredicate(list->head->payload, tempPathB) == 0;

    zArchiveList *created = zUtil_ZRDR_CreateSearchPathList(tempPathA);
    const bool createdOk =
        created != nullptr && created->count == 1 &&
        std::strcmp(static_cast<const char *>(created->head->payload), tempPathA) == 0;

    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return listOk && createdOk ? 0 : 2;
}

extern "C" int zreader_zrdr_free_search_path_list_smoke(void) {
    EnsureZrdrFreePool();

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zfl", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zfm", 0, tempPathB) == 0) {
        return 1;
    }

    char searchText[MAX_PATH * 3] = {};
    std::strcpy(searchText, tempPathA);
    std::strcat(searchText, ";");
    std::strcat(searchText, tempPathB);

    const int oldFreeCount = g_zUtil_ZRDR_FreePool->count;
    zArchiveList *const list = zUtil_ZRDR_CreateSearchPathList(searchText);
    if (list == nullptr || list->count != 2) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    const int result = zUtil_ZRDR_FreeSearchPathList(list);
    const bool ok = result == 0 && g_zUtil_ZRDR_FreePool != nullptr &&
                    g_zUtil_ZRDR_FreePool->count >= oldFreeCount + 2;

    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return ok ? 0 : 3;
}

extern "C" int zreader_prealloc_and_pop_front_smoke(void) {
    g_zUtil_ZRDR_FreePool = nullptr;
    g_zUtil_ZRDR_TotalAllocated = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;

    zUtil::ZRDR_PreallocNodePool(2);
    if (g_zUtil_ZRDR_FreePool == nullptr || g_zUtil_ZRDR_FreePool->count != 2 ||
        g_zUtil_ZRDR_TotalAllocated != 2 || g_zUtil_ZRDR_FreeCount != 2) {
        return 1;
    }

    zArchiveList *list = zArchiveList_CreateEmpty();
    char payloadA[] = "a";
    char payloadB[] = "b";
    zArchiveList_PushFrontPayload(list, payloadA);
    zArchiveList_PushFrontPayload(list, payloadB);

    const bool popOk = list->count == 2 && zArchiveList_PopFrontPayload(list) == payloadB &&
                       list->count == 1 && list->head->payload == payloadA &&
                       zArchiveList_PopFrontPayload(list) == payloadA && list->count == 0 &&
                       list->head == nullptr && zArchiveList_PopFrontPayload(list) == nullptr &&
                       zArchiveList_FreeNode(nullptr) == nullptr &&
                       g_zUtil_ZRDR_FreePool->count == 2;

    struct ValuePayload {
        std::uint32_t value;
        const char *name;
    };

    ValuePayload valueA{0x10, "a"};
    ValuePayload valueB{0x20, "b"};
    ValuePayload valueC{0x30, "c"};
    zArchiveList *valueList = zArchiveList_CreateEmpty();
    zArchiveList_PushBackPayload(valueList, &valueA);
    zArchiveList_PushBackPayload(valueList, &valueB);
    zArchiveList_PushBackPayload(valueList, &valueC);

    const bool removeOk =
        zArchiveList_FindPayloadByValue(valueList, 0x20) == &valueB &&
        zArchiveList_FindNodeByPayload(valueList, &valueB) == valueList->head->next &&
        zArchiveList_RemovePayload(valueList, &valueB) == 2 &&
        valueList->head->payload == &valueA && valueList->head->next->payload == &valueC &&
        zArchiveList_FindPayloadByValue(valueList, 0x20) == nullptr &&
        zArchiveList_RemovePayload(valueList, &valueA) == 1 &&
        valueList->head->payload == &valueC &&
        zArchiveList_RemovePayload(valueList, &valueC) == 0 && valueList->head == nullptr &&
        zArchiveList_RemovePayload(valueList, &valueC) == -1;

    std::free(list);
    std::free(valueList);
    return popOk && removeOk ? 0 : 2;
}

extern "C" int zreader_zrdr_init_search_path_smoke(void) {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zri", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zrj", 0, tempPathB) == 0) {
        return 1;
    }

    g_zArchive_MountedList = nullptr;
    g_zArchive_Current = reinterpret_cast<zArchiveList *>(0x12345678);
    g_zRdr_SearchPathList = nullptr;

    zUtil_ZRDR_AppendSearchPath(tempPathA);
    const bool appendCreateOk =
        g_zRdr_SearchPathList != nullptr && g_zRdr_SearchPathList->count == 1 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload), tempPathA) ==
            0;

    if (zUtil::ZRDR_Init(tempPathA) != 0 || g_zArchive_MountedList == nullptr ||
        g_zArchive_Current != nullptr || g_zRdr_SearchPathList == nullptr ||
        g_zRdr_SearchPathList->count != 1 ||
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload), tempPathA) !=
            0) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    zUtil_ZRDR_SetSearchPath(tempPathB);
    const bool setOk = g_zRdr_SearchPathList->count == 1 &&
                       std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload),
                                   tempPathB) == 0;

    zUtil_ZRDR_AppendSearchPath(tempPathA);
    const bool appendOk =
        g_zRdr_SearchPathList->count == 2 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->payload), tempPathA) ==
            0 &&
        std::strcmp(static_cast<const char *>(g_zRdr_SearchPathList->head->next->payload),
                    tempPathB) == 0;

    zUtil_ZRDR_FreePathList(g_zRdr_SearchPathList);
    std::free(g_zRdr_SearchPathList);
    g_zRdr_SearchPathList = nullptr;
    if (g_zRdr_ScratchSearchPathList != nullptr) {
        zUtil_ZRDR_FreePathList(g_zRdr_ScratchSearchPathList);
        std::free(g_zRdr_ScratchSearchPathList);
        g_zRdr_ScratchSearchPathList = nullptr;
    }
    std::free(g_zArchive_MountedList);
    g_zArchive_MountedList = nullptr;

    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return appendCreateOk && setOk && appendOk ? 0 : 3;
}

extern "C" int zreader_zrdr_shutdown_smoke(void) {
    EnsureZrdrFreePool();

    char tempDir[MAX_PATH] = {};
    char tempPathA[MAX_PATH] = {};
    char tempPathB[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zrs", 0, tempPathA) == 0 ||
        GetTempFileNameA(tempDir, "zrt", 0, tempPathB) == 0) {
        return 1;
    }

    g_zRdr_SearchPathList = zUtil_ZRDR_CreateSearchPathList(tempPathA);
    g_zRdr_ScratchSearchPathList = zUtil_ZRDR_CreateSearchPathList(tempPathB);
    g_zArchive_MountedList = zArchiveList_CreateEmpty();

    if (zUtil_ZRDR_ShutdownWildcardPath() != 0 || g_zRdr_ScratchSearchPathList != nullptr) {
        DeleteFileA(tempPathA);
        DeleteFileA(tempPathB);
        return 2;
    }

    const int shutdownResult = zUtil_ZRDR_Shutdown();
    DeleteFileA(tempPathA);
    DeleteFileA(tempPathB);
    return shutdownResult == 0 && g_zRdr_SearchPathList == nullptr &&
                   g_zArchive_MountedList == nullptr
               ? 0
               : 3;
}

extern "C" int zreader_zrdr_free_node_pool_smoke(void) {
    g_zUtil_ZRDR_FreePool = nullptr;
    g_zUtil_ZRDR_TotalAllocated = 0;
    g_zUtil_ZRDR_FreeCount = 0;
    g_zUtil_ZRDR_GrowCount = 0;

    zUtil::ZRDR_PreallocNodePool(3);
    if (g_zUtil_ZRDR_FreePool == nullptr || g_zUtil_ZRDR_FreePool->count != 3) {
        return 1;
    }

    zUtil_ZRDR_FreeNodePool();
    return g_zUtil_ZRDR_FreePool == nullptr ? 0 : 2;
}

extern "C" int zreader_mount_index_archive_smoke(void) {
    if (g_zUtil_ZRDR_FreePool == nullptr) {
        g_zUtil_ZRDR_FreePool = zArchiveList_CreateEmpty();
    }

    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zrm", 0, tempPath) == 0) {
        return 1;
    }

    HANDLE file = CreateFileA(tempPath, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 2;
    }

    const char payload[] = "payload";
    DWORD written = 0;
    WriteFile(file, payload, sizeof(payload) - 1, &written, nullptr);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = sizeof(payload) - 1;
    std::strcpy(record.name, "payload.bin");
    WriteFile(file, &record, sizeof(record), &written, nullptr);
    WriteU32(file, 1);
    WriteU32(file, 1);
    CloseHandle(file);

    g_zArchive_MountedList = zArchiveList_CreateEmpty();
    g_zArchive_Current = nullptr;

    const bool mountOk = zArchive::MountIndexArchive(tempPath, 1) == 1 &&
                         g_zArchive_MountedList != nullptr && g_zArchive_MountedList->count == 1 &&
                         g_zArchive_Current != nullptr &&
                         g_zArchive_Current == g_zArchive_MountedList->head->payload;

    zIndexArchive *archive = reinterpret_cast<zIndexArchive *>(g_zArchive_Current);
    std::uint32_t openedSize = 0;
    const bool archiveOk =
        mountOk && archive->recordCount == 1 &&
        archive->FindRecordByNameCI("PAYLOAD.BIN") != nullptr &&
        archive->OpenFileByName("payload.bin", &openedSize) != INVALID_HANDLE_VALUE &&
        openedSize == sizeof(payload) - 1;

    zUtil_ZRDR_UnloadMountedArchives(1);
    zArchiveList_Destroy(g_zArchive_MountedList);
    g_zArchive_MountedList = nullptr;
    g_zArchive_Current = nullptr;
    DeleteFileA(tempPath);
    return mountOk && archiveOk ? 0 : 3;
}

extern "C" int zreader_index_archive_flush_close_smoke(void) {
    char tempDir[MAX_PATH] = {};
    char tempPath[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0 ||
        GetTempFileNameA(tempDir, "zif", 0, tempPath) == 0) {
        return 1;
    }

    zIndexArchive archive = {};
    archive.Reset();
    if (archive.OpenCreateWrite(tempPath) == 0) {
        DeleteFileA(tempPath);
        return 2;
    }

    zZarFileRecord *records =
        static_cast<zZarFileRecord *>(std::malloc(sizeof(zZarFileRecord)));
    if (records == nullptr) {
        archive.CloseAndFreeRecords();
        DeleteFileA(tempPath);
        return 3;
    }

    std::memset(records, 0, sizeof(zZarFileRecord));
    records[0].fileOffset = 12;
    records[0].fileSize = 34;
    std::strcpy(records[0].name, "flush.bin");

    archive.records = records;
    archive.recordCount = 1;
    archive.recordCapacity = 1;
    archive.dirty = 1;

    const bool closeOk = archive.CloseAndFreeRecords() == 1 &&
                         archive.hFile == INVALID_HANDLE_VALUE && archive.records == nullptr &&
                         archive.recordCount == 0 && archive.recordCapacity == 0 &&
                         archive.dirty == 0 && archive.reservedFree == nullptr;

    HANDLE readFile = CreateFileA(tempPath, GENERIC_READ, FILE_SHARE_READ, nullptr,
                                  OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (readFile == INVALID_HANDLE_VALUE) {
        DeleteFileA(tempPath);
        return 4;
    }

    const DWORD fileSize = GetFileSize(readFile, nullptr);
    SetFilePointer(readFile, -8, nullptr, FILE_END);
    std::uint32_t footerMagic = 0;
    std::uint32_t footerCount = 0;
    DWORD bytesRead = 0;
    ReadFile(readFile, &footerMagic, sizeof(footerMagic), &bytesRead, nullptr);
    ReadFile(readFile, &footerCount, sizeof(footerCount), &bytesRead, nullptr);
    CloseHandle(readFile);

    zIndexArchive partial = {};
    partial.Reset();
    partial.reservedFree = std::malloc(4);
    partial.Destroy();

    DeleteFileA(tempPath);
    return closeOk && fileSize == sizeof(zZarFileRecord) + 8 && footerMagic == 1 &&
                   footerCount == 1
               ? 0
               : 5;
}

extern "C" int zreader_free_loaded_tree_smoke(void) {
    zReader::Node stringNode{};
    stringNode.type = zReader::ZRDR_NODE_STRING;
    stringNode.value.str = _strdup("leaf");
    if (stringNode.value.str == nullptr) {
        return 1;
    }

    zReader_FreeNodeRecursive(&stringNode);
    if (stringNode.value.str != nullptr) {
        return 2;
    }

    zReader::Node *root = static_cast<zReader::Node *>(std::malloc(sizeof(zReader::Node)));
    zReader::Node *arrayBase = static_cast<zReader::Node *>(std::malloc(3 * sizeof(zReader::Node)));
    if (root == nullptr || arrayBase == nullptr) {
        std::free(root);
        std::free(arrayBase);
        return 3;
    }

    root->type = zReader::ZRDR_NODE_ARRAY;
    root->value.nodes = arrayBase;
    arrayBase[0].type = zReader::ZRDR_NODE_INT;
    arrayBase[0].value.i32 = 3;
    arrayBase[1].type = zReader::ZRDR_NODE_STRING;
    arrayBase[1].value.str = _strdup("child");
    arrayBase[2].type = zReader::ZRDR_NODE_INT;
    arrayBase[2].value.i32 = 7;

    if (arrayBase[1].value.str == nullptr) {
        std::free(arrayBase);
        std::free(root);
        return 4;
    }

    return zReader::FreeLoadedTree(root);
}

extern "C" int zreader_load_movers_from_zrd_smoke(void) {
    char tempPath[MAX_PATH] = {};
    GetTempPathA(MAX_PATH, tempPath);
    std::strcat(tempPath, "recoil_movers_test.zar");

    HANDLE file = CreateFileA(tempPath, GENERIC_READ | GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                              FILE_ATTRIBUTE_TEMPORARY, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return 1;
    }

    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 2);
    WriteU32(file, zReader::ZRDR_NODE_ARRAY);
    WriteU32(file, 3);
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, 5);
    WriteBytes(file, "alpha", 5);
    WriteU32(file, zReader::ZRDR_NODE_STRING);
    WriteU32(file, 4);
    WriteBytes(file, "beta", 4);

    zZarFileRecord record = {};
    record.fileOffset = 0;
    record.fileSize = SetFilePointer(file, 0, nullptr, FILE_CURRENT);
    std::strcpy(record.name, "movers.zrd");

    zIndexArchive archive = {};
    archive.hFile = file;
    archive.recordCount = 1;
    archive.records = &record;

    zArchiveListNode archiveNode = {};
    archiveNode.payload = &archive;
    archiveNode.next = &archiveNode;
    archiveNode.prev = &archiveNode;
    zArchiveList mountedList = {};
    mountedList.count = 1;
    mountedList.head = &archiveNode;
    g_zArchive_MountedList = &mountedList;

    zClass_NodePartial alpha{};
    zClass_NodePartial beta{};
    zClass_NodePartial child{};
    zClass_NodePartial *betaChildren[] = {&child};
    std::strcpy(alpha.name, "alpha");
    std::strcpy(beta.name, "beta");
    beta.listCountB = 1;
    beta.listB = betaChildren;
    zClass_TypeListLink betaLink{&beta, nullptr, nullptr, 0};
    zClass_TypeListLink alphaLink{&alpha, nullptr, &betaLink, 0};
    for (std::int32_t i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
    }
    zClass_TypeList::Head(6) = &alphaLink;
    g_Mover_LastLoadedNode = nullptr;

    zReader::LoadMoversFromZrd();

    const bool ok = (alpha.auxFlags & 1) != 0 && (beta.auxFlags & 1) != 0 &&
                    (child.auxFlags & 1) != 0 && (alpha.flags & 0x200000) != 0 &&
                    (beta.flags & 0x200000) != 0 && (child.flags & 0x200000) != 0 &&
                    alpha.callbackContext == &alpha && beta.callbackContext == &beta &&
                    child.callbackContext == &beta && g_Mover_LastLoadedNode == &beta;

    zClass_TypeList::Head(6) = nullptr;
    g_zArchive_MountedList = nullptr;
    CloseHandle(file);
    DeleteFileA(tempPath);
    return ok ? 0 : 2;
}

extern "C" int zreader_resolve_and_open_file_smoke(void) {
    char tempDir[MAX_PATH] = {};
    if (GetTempPathA(sizeof(tempDir), tempDir) == 0) {
        return 1;
    }

    char searchDir[MAX_PATH] = {};
    std::strcpy(searchDir, tempDir);
    const std::size_t dirLen = std::strlen(searchDir);
    if (dirLen != 0 && searchDir[dirLen - 1] == '\\') {
        searchDir[dirLen - 1] = '\0';
    }

    char filePath[MAX_PATH] = {};
    std::sprintf(filePath, "%s\\%s", searchDir, "zrdr_resolve.tmp");
    {
        FILE *out = std::fopen(filePath, "wb");
        if (out == nullptr) {
            return 2;
        }
        std::fputs("ok", out);
        std::fclose(out);
    }

    EnsureZrdrFreePool();
    zArchiveList *list = zArchiveList_CreateEmpty();
    zArchiveList_PushFrontPayload(list, searchDir);

    const char *direct = zReader::TryResolvePath(filePath, nullptr);
    const char *viaExtra = zReader::TryResolvePath("ignored\\zrdr_resolve.tmp", searchDir);
    char *resolved = zUtil_ZRDR_ResolvePathInSearchPathList(list, "ignored\\zrdr_resolve.tmp");
    g_zRdr_SearchPathList = list;
    const char *viaGlobal = zReader::TryResolvePath("ignored\\zrdr_resolve.tmp", "");
    FILE *in = zUtil_ZRDR_OpenFileResolved(list, "ignored\\zrdr_resolve.tmp", "rb");
    char parentDir[0x104] = {};
    char expectedFullPath[0x104] = {};
    char expectedDrive[3] = {};
    char expectedDir[0x100] = {};
    char expectedName[0x100] = {};
    char expectedExt[0x100] = {};
    zReader::BuildResolvedParentDir(filePath, parentDir);
    _fullpath(expectedFullPath, filePath, sizeof(expectedFullPath));
    _splitpath(expectedFullPath, expectedDrive, expectedDir, expectedName, expectedExt);
    char expectedParentDir[0x104] = {};
    std::sprintf(expectedParentDir, "%s%s", expectedDrive, expectedDir);

    const bool ok = direct == filePath && viaExtra != nullptr &&
                    std::strcmp(viaExtra, filePath) == 0 && viaGlobal != nullptr &&
                    std::strcmp(viaGlobal, filePath) == 0 && resolved != nullptr &&
                    std::strcmp(resolved, filePath) == 0 && in != nullptr &&
                    std::strcmp(parentDir, expectedParentDir) == 0;
    if (in != nullptr) {
        std::fclose(in);
    }

    g_zRdr_SearchPathList = nullptr;
    zArchiveList_PopFrontPayload(list);
    std::free(list);
    DeleteFileA(filePath);
    return ok ? 0 : 3;
}
