#include "zReader.h"

#include <string.h>

// Reimplements 0x48cec0: zReader_FindChildRecursive
extern "C" RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL
zReader_FindChildRecursive(zReader::Node *node, const char *searchName, int startIndex) {
    if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *arrayBase = node->value.nodes;
    if (startIndex >= arrayBase->value.i32) {
        return 0;
    }

    while (startIndex < node->value.nodes->value.i32) {
        zReader::Node *child = &arrayBase[startIndex];
        int childType = child->type;
        if (childType == zReader::ZRDR_NODE_ARRAY) {
            zReader::Node *result = zReader_FindChildRecursive(child, searchName, 1);
            if (result != 0) {
                return result;
            }
        }
        else if (childType == zReader::ZRDR_NODE_STRING &&
            strcmp(child->value.str, searchName) == 0) {
            return &child[1];
        }

        ++startIndex;
    }

    return 0;
}

// Reimplements 0x48cf70: zReader_GetNamedNode
extern "C" RECOIL_NOINLINE zReader::Node *RECOIL_FASTCALL
zReader_GetNamedNode(zReader::Node *parentNode, const char *name) {
    return zReader_FindChildRecursive(parentNode, name, 1);
}

namespace zReader {
// Reimplements 0x48cf80: zReader::ReadNamedString
RECOIL_NOINLINE const char *RECOIL_FASTCALL ReadNamedString(Node *parentNode, const char *name) {
    Node *node = zReader_GetNamedNode(parentNode, name);
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_STRING) {
        return node->value.str;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_STRING) {
            return arrayBase[1].value.str;
        }
    }

    return 0;
}

// Reimplements 0x48cfb0: zReader::ReadNamedFloat
RECOIL_NOINLINE int RECOIL_FASTCALL ReadNamedFloat(Node *parentNode, const char *name,
                                                            float *outValue) {
    Node *node = zReader_GetNamedNode(parentNode, name);
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_FLOAT) {
        *outValue = node->value.f32;
        return 1;
    }

    if (node->type == ZRDR_NODE_INT) {
        *outValue = static_cast<float>(node->value.i32);
        return 1;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_FLOAT) {
            *outValue = arrayBase[1].value.f32;
            return 1;
        }

        if (arrayBase[1].type == ZRDR_NODE_INT) {
            *outValue = static_cast<float>(arrayBase[1].value.i32);
            return 1;
        }
    }

    return 0;
}

// Reimplements 0x48d030: zReader::ReadNamedInt
RECOIL_NOINLINE int RECOIL_FASTCALL ReadNamedInt(Node *parentNode, const char *name,
                                                          int *outValue) {
    Node *node = zReader_GetNamedNode(parentNode, name);
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_INT) {
        *outValue = node->value.i32;
        return 1;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_INT) {
            *outValue = arrayBase[1].value.i32;
            return 1;
        }
    }

    return 0;
}
} // namespace zReader
