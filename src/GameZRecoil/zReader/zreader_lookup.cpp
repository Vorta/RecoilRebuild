#include "zReader.h"

#include <ctype.h>
#include <string.h>

/**
 * Reimplements 0x48cec0: zReader_FindChildRecursive (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Recursively finds a named zReader child and returns the value node adjacent to the matching name string.
 */
extern "C" zReader::Node *__fastcall zReader_FindChildRecursive(
    zReader::Node *node,
    const char *searchName,
    int startIndex
) {
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
            zReader::Node *result = zReader_FindChildRecursive(
                child,
                searchName,
                1
            );
            if (result != 0) {
                return result;
            }
        } else if (childType == zReader::ZRDR_NODE_STRING &&
                   strcmp(
                       child->value.str,
                       searchName
                   ) == 0) {
            return &child[1];
        }

        ++startIndex;
    }

    return 0;
}

/**
 * Reimplements 0x48cf70: zReader_GetNamedNode (GameZRecoil/zReader/zreader.cpp).
 * Purpose: Starts the recursive named-node lookup at the first payload child of an array node.
 */
extern "C" zReader::Node *__fastcall zReader_GetNamedNode(
    zReader::Node *parentNode,
    const char *name
) {
    return zReader_FindChildRecursive(
        parentNode,
        name,
        1
    );
}

namespace zReader {
/**
 * Reimplements 0x4804e0: zReader::FindGlobalStringPrefixIndex
 * (Battlesport/zUtil/zrdr_global.c).
 *
 * Purpose: find the global string-table prefix that matches the start of a
 * reader token and stops before the next alphanumeric character.
 */
int __fastcall FindGlobalStringPrefixIndex(
    const char *text
) {
    if (text == 0) {
        return -1;
    }

    for (int index = 0; index < g_zRndr_GlobalStringCount; ++index) {
        const char *const prefix = g_zRndr_GlobalStringTable[index];
        const size_t prefixLength = strlen(prefix);
        if (strlen(text) < prefixLength) {
            continue;
        }

        const unsigned char nextChar = (unsigned char)(text[prefixLength]);
        if (nextChar != '\0' && _isctype(
            nextChar,
            0x0008
        ) == 0) {
            continue;
        }

        if (_strnicmp(
            text,
            prefix,
            prefixLength
        ) == 0) {
            return index;
        }
    }

    return -1;
}

/**
 * Reimplements 0x48cf80: zReader::ReadNamedString.
 *
 * Purpose: read a named string value from a node or the first payload item of a
 * named array node.
 */
const char *__fastcall ReadNamedString(
    Node *parentNode,
    const char *name
) {
    Node *node = zReader_GetNamedNode(
        parentNode,
        name
    );
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

/**
 * Reimplements 0x48cfb0: zReader::ReadNamedFloat.
 *
 * Purpose: read a named float value, accepting integer nodes as float-compatible
 * values when the source data stores the number as an int.
 */
int __fastcall ReadNamedFloat(
    Node *parentNode,
    const char *name,
    float *outValue
) {
    Node *node = zReader_GetNamedNode(
        parentNode,
        name
    );
    if (node == 0) {
        return 0;
    }

    if (node->type == ZRDR_NODE_FLOAT) {
        *outValue = node->value.f32;
        return 1;
    }

    if (node->type == ZRDR_NODE_INT) {
        *outValue = (float)(node->value.i32);
        return 1;
    }

    if (node->type == ZRDR_NODE_ARRAY) {
        Node *arrayBase = node->value.nodes;
        if (arrayBase[1].type == ZRDR_NODE_FLOAT) {
            *outValue = arrayBase[1].value.f32;
            return 1;
        }

        if (arrayBase[1].type == ZRDR_NODE_INT) {
            *outValue = (float)(arrayBase[1].value.i32);
            return 1;
        }
    }

    return 0;
}

/**
 * Reimplements 0x48d030: zReader::ReadNamedInt.
 *
 * Purpose: read a named integer value from a node or the first payload item of
 * a named array node.
 */
int __fastcall ReadNamedInt(
    Node *parentNode,
    const char *name,
    int *outValue
) {
    Node *node = zReader_GetNamedNode(
        parentNode,
        name
    );
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
