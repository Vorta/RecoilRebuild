/*
 * zreader.cpp — ZRD file reader subsystem
 * Original: D:\Proj\GameZRecoil\zReader\zreader.cpp
 *
 * Reconstructed from Recoil.exe (1998 Zipper Interactive)
 * Binary Ninja analysis of functions at 0x48CD40–0x48D1C0
 *
 * ZRD Binary Format:
 *   Each node is { int32_t type; union value; }
 *   Type 1 = int32   (value is 4 bytes)
 *   Type 2 = float   (value is 4 bytes)
 *   Type 3 = string  (value is length-prefixed string via zReader_ReadString)
 *   Type 4 = array   (value is child count + recursive child nodes)
 *
 *   Name-value pairs are encoded as consecutive string+value nodes in arrays.
 *   e.g. array[ "health", 100, "name", "Tank" ] = two key-value pairs.
 */

#define _CRT_SECURE_NO_WARNINGS

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

/* ---- Types ---- */

/* Node type tags */
#define ZRDR_TYPE_INT 1
#define ZRDR_TYPE_FLOAT 2
#define ZRDR_TYPE_STRING 3
#define ZRDR_TYPE_ARRAY 4

union zReader_Value {
    uint32_t u32;
    int32_t i32;
    float f32;
    char *str;
    void *ptr;
};

/* 8 bytes per node */
struct zReader_Node {
    int32_t type_00; /* ZRDR_TYPE_* */
    zReader_Value value_04;
};

/* Array header: first node in malloc'd block stores count in value_04 */
struct zReader_NodeArray {
    int32_t countTag_00;      /* always 1 (type=int) */
    int32_t count_04;         /* number of child nodes */
    zReader_Node nodes_08[1]; /* variable-length array of children */
};

/* ---- Globals (extern) ---- */

extern char g_zReader_FileNameBuf[]; /* _splitpath filename buffer */
extern char g_zReader_FileExtBuf[];  /* _splitpath extension buffer */
extern void *g_zArchive_MountedList; /* mounted archive index list */
extern void *g_zArchive_Current;     /* current archive handle */
extern void *g_zRdr_SearchPathList;  /* global search path list */

/* ---- Forward declarations (zUtil_ZRDR layer) ---- */

extern void *zUtil_ZRDR_InitFileIndexList(void);
extern void zUtil_ZRDR_SetSearchPath(const char *searchPath);
extern void *zUtil_ZRDR_CreateSearchPathList(const char *pathText);
extern void zUtil_ZRDR_FreeSearchPathList(void *list);
extern const char *zUtil_ZRDR_ResolvePathInSearchPathList(
    void *list,
    const char *file
);

extern int zReader_FileExists(const char *path);
extern int zReader_ReadString(
    HANDLE hFile,
    zReader_Value *outValue
); /* 0x4A6110 */
extern int zArchiveList_GetCount(void *list);
extern void *zArchiveList_GetAt(void *list);
extern int zIndexArchive_OpenFile(void *archive);

/* ---- zReader core (implemented later in this file) ---- */
int __cdecl zReader_OpenFileFromMountedArchives(const char *filename);
int __cdecl zReader_ReadNode(
    HANDLE hFile,
    struct zReader_Node *outNode
);
void __cdecl zReader_FreeNodeRecursive(struct zReader_Node *node);

/**
 * Reimplements 0x48cc70: zUtil_ZRDR_Init.
 * Purpose: initializes the ZRD reader archive list, search path, and current
 * archive state.
 */
int __cdecl zUtil_ZRDR_Init(
    const char *searchPath
) {
    if (g_zArchive_MountedList == NULL) {
        g_zArchive_MountedList = zUtil_ZRDR_InitFileIndexList();
    }
    zUtil_ZRDR_SetSearchPath(searchPath);
    g_zArchive_Current = NULL;
    return 0;
}

/**
 * Reimplements 0x48cd40: zReader_TryResolvePath.
 * Purpose: resolves a requested ZRD path directly, through an optional search
 * path list, or through the global reader search path list.
 */
const char *__cdecl zReader_TryResolvePath(
    const char *file,
    const char *optionalSearchPathListText
) {
    const char *result = NULL;

    if (zReader_FileExists(file)) {
        result = file;
    }

    if (result == NULL) {
        if (optionalSearchPathListText != NULL && strlen(optionalSearchPathListText) > 0) {
            void *pathList = zUtil_ZRDR_CreateSearchPathList(optionalSearchPathListText);
            result = zUtil_ZRDR_ResolvePathInSearchPathList(
                pathList,
                file
            );
            zUtil_ZRDR_FreeSearchPathList(pathList);
        }

        if (result == NULL) {
            return zUtil_ZRDR_ResolvePathInSearchPathList(
                g_zRdr_SearchPathList,
                file
            );
        }
    }

    return result;
}

/**
 * Reimplements 0x48cda0: zReader_AllocateNode.
 * Purpose: allocates a node block and stores the caller-supplied node type in
 * the first node.
 */
zReader_Node *__cdecl zReader_AllocateNode(
    int type,
    int count
) {
    zReader_Node *result = (zReader_Node *)malloc(count * sizeof(zReader_Node));
    result->type_00 = type;
    return result;
}

/**
 * Reimplements 0x48cdc0: zReader_LoadNodeFromPath.
 * Purpose: strips a path to mounted-archive filename form, opens it, and
 * deserializes the root ZRD node tree.
 */
zReader_Node *__cdecl zReader_LoadNodeFromPath(
    const char *path
) {
    zReader_Node *outNode = NULL;

    _splitpath(
        path,
        NULL,
        NULL,
        g_zReader_FileNameBuf,
        g_zReader_FileExtBuf
    );

    /* Append extension to filename (inline strlen + memcpy) */
    strcat(
        g_zReader_FileNameBuf,
        g_zReader_FileExtBuf
    );

    HANDLE hFile = (HANDLE)zReader_OpenFileFromMountedArchives(g_zReader_FileNameBuf);
    if (hFile != INVALID_HANDLE_VALUE) {
        outNode = zReader_AllocateNode(
            ZRDR_TYPE_ARRAY,
            1
        );
        zReader_ReadNode(
            hFile,
            outNode
        );
    }

    return outNode;
}

/**
 * Reimplements 0x48ce40: zReader_FreeLoadedTree.
 * Purpose: recursively frees a loaded ZRD tree and releases the root node.
 */
void __cdecl zReader_FreeLoadedTree(
    zReader_Node *loaded
) {
    if (loaded != NULL) {
        zReader_FreeNodeRecursive(loaded);
        free(loaded);
    }
}

/**
 * Reimplements 0x48ce60: zReader_FreeNodeRecursive.
 * Purpose: recursively releases string and array payloads owned by a ZRD node.
 */
void __cdecl zReader_FreeNodeRecursive(
    zReader_Node *node
) {
    switch (node->type_00) {
    case ZRDR_TYPE_STRING:
        free(node->value_04.str);
        break;

    case ZRDR_TYPE_ARRAY: {
        zReader_NodeArray *arr = (zReader_NodeArray *)node->value_04.ptr;
        int count = arr->count_04;
        for (int i = 1; i < count; i++) {
            zReader_FreeNodeRecursive(&arr->nodes_08[i - 1]);
        }
        free(arr);
        break;
    }
    }
}

/**
 * Reimplements 0x48cec0: zReader_FindChildRecursive.
 * Purpose: searches nested array nodes for a named string key and returns the
 * following value node.
 */
zReader_Node *__cdecl zReader_FindChildRecursive(
    zReader_Node *self,
    const char *name,
    int startIndex
) {
    if (self == NULL || self->type_00 != ZRDR_TYPE_ARRAY) {
        return NULL;
    }

    zReader_NodeArray *base = (zReader_NodeArray *)self->value_04.ptr;
    {
        for (int idx = startIndex; idx < base->count_04; idx++) {
            zReader_Node *childNode = (zReader_Node *)&base->nodes_08[idx - 1];
            int type = childNode->type_00;

            if (type == ZRDR_TYPE_ARRAY) {
                zReader_Node *result = zReader_FindChildRecursive(
                    childNode,
                    name,
                    1
                );
                if (result != NULL) {
                    return result;
                }
            }

            if (type == ZRDR_TYPE_STRING) {
                if (strcmp(
                    childNode->value_04.str,
                    name
                ) == 0) {
                    return &childNode[1]; /* return the value node after the name */
                }
            }
        }
    }

    return NULL;
}

/**
 * Reimplements 0x48cf70: zReader_GetCurrentRootNode.
 * Purpose: finds a named child value from the current root using the standard
 * first-child search index.
 */
zReader_Node *__cdecl zReader_GetCurrentRootNode(
    zReader_Node *parent,
    const char *name
) {
    return zReader_FindChildRecursive(
        parent,
        name,
        1
    );
}

/**
 * Reimplements 0x48cf80: zReader_ReadNamedString.
 * Purpose: looks up a named value and returns its string payload, including
 * the first string inside array-valued nodes.
 */
const char *__cdecl zReader_ReadNamedString(
    zReader_Node *parent,
    const char *name
) {
    zReader_Node *node = zReader_GetCurrentRootNode(
        parent,
        name
    );
    if (node == NULL)
        return NULL;

    if (node->type_00 == ZRDR_TYPE_STRING) {
        return node->value_04.str;
    }

    if (node->type_00 == ZRDR_TYPE_ARRAY) {
        zReader_NodeArray *arr = (zReader_NodeArray *)node->value_04.ptr;
        if (arr->nodes_08[0].type_00 == ZRDR_TYPE_STRING) {
            return arr->nodes_08[0].value_04.str;
        }
    }

    return NULL;
}

/**
 * Reimplements 0x48cfb0: zReader_ReadNamedFloat.
 * Purpose: looks up a named value and writes its float form from direct or
 * array-contained float/int nodes.
 */
int __cdecl zReader_ReadNamedFloat(
    zReader_Node *parent,
    const char *name,
    float *outValue
) {
    zReader_Node *node = zReader_GetCurrentRootNode(
        parent,
        name
    );
    if (node == NULL)
        return 0;

    int type = node->type_00;

    if (type == ZRDR_TYPE_FLOAT) {
        *outValue = node->value_04.f32;
        return 1;
    }

    if (type == ZRDR_TYPE_INT) {
        *outValue = (float)node->value_04.i32;
        return 1;
    }

    if (type == ZRDR_TYPE_ARRAY) {
        zReader_NodeArray *arr = (zReader_NodeArray *)node->value_04.ptr;
        if (arr->nodes_08[0].type_00 == ZRDR_TYPE_FLOAT) {
            *outValue = arr->nodes_08[0].value_04.f32;
            return 1;
        }
        if (arr->nodes_08[0].type_00 == ZRDR_TYPE_INT) {
            *outValue = (float)arr->nodes_08[0].value_04.i32;
            return 1;
        }
    }

    return 0;
}

/**
 * Reimplements 0x48d030: zReader_ReadNamedInt.
 * Purpose: looks up a named value and writes its integer form from direct or
 * array-contained integer nodes.
 */
int __cdecl zReader_ReadNamedInt(
    zReader_Node *parent,
    const char *name,
    int *outValue
) {
    zReader_Node *node = zReader_GetCurrentRootNode(
        parent,
        name
    );
    if (node == NULL)
        return 0;

    int type = node->type_00;

    if (type == ZRDR_TYPE_INT) {
        *outValue = node->value_04.i32;
        return 1;
    }

    if (type == ZRDR_TYPE_ARRAY) {
        zReader_NodeArray *arr = (zReader_NodeArray *)node->value_04.ptr;
        if (arr->nodes_08[0].type_00 == ZRDR_TYPE_INT) {
            *outValue = arr->nodes_08[0].value_04.i32;
            return 1;
        }
    }

    return 0;
}

/**
 * Reimplements 0x48d080: zReader_ReadNode.
 * Purpose: deserializes one typed ZRD node from an open file handle,
 * recursively reading child array nodes.
 */
int __cdecl zReader_ReadNode(
    HANDLE hFile,
    zReader_Node *outNode
) {
    DWORD bytesRead;
    ReadFile(
        hFile,
        outNode,
        4,
        &bytesRead,
        NULL
    ); /* read type tag */
    DWORD totalBytes = bytesRead;

    switch (outNode->type_00) {
    case ZRDR_TYPE_INT:
        ReadFile(
            hFile,
            &outNode->value_04,
            4,
            &bytesRead,
            NULL
        );
        return totalBytes + bytesRead;

    case ZRDR_TYPE_FLOAT:
        ReadFile(
            hFile,
            &outNode->value_04,
            4,
            &bytesRead,
            NULL
        );
        return totalBytes + bytesRead;

    case ZRDR_TYPE_STRING:
        return totalBytes + zReader_ReadString(
            hFile,
            &outNode->value_04
        );

    case ZRDR_TYPE_ARRAY: {
        int nodeCount;
        ReadFile(
            hFile,
            &nodeCount,
            4,
            &bytesRead,
            NULL
        );
        totalBytes += bytesRead;

        /* Allocate array: first node is header with count, rest are children */
        zReader_Node *arr = (zReader_Node *)malloc(nodeCount * sizeof(zReader_Node));
        outNode->value_04.ptr = arr;
        arr->value_04.i32 = nodeCount; /* store count in header */
        arr->type_00 = ZRDR_TYPE_INT;  /* header tag */

        for (int i = 1; i < nodeCount; i++) {
            totalBytes += zReader_ReadNode(
                hFile,
                &arr[i]
            );
        }
        return totalBytes;
    }

    default:
        /* zError: "Invalid reader node type in zRdrRead()" */
        return 0;
    }
}

/**
 * Reimplements 0x48d1c0: zReader_OpenFileFromMountedArchives.
 * Purpose: scans mounted archives and returns the first archive file handle
 * for the requested filename.
 */
int __cdecl zReader_OpenFileFromMountedArchives(
    const char *filename
) {
    if (g_zArchive_MountedList == NULL) {
        return -1;
    }

    int count = zArchiveList_GetCount(g_zArchive_MountedList);
    for (int i = 0; i < count; i++) {
        void *archive = zArchiveList_GetAt(g_zArchive_MountedList);
        int result = zIndexArchive_OpenFile(archive /*, filename, 0 */);
        if (result != -1) {
            return result;
        }
    }

    return -1;
}
