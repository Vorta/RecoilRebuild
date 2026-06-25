#include <string.h>

#include "messages_lookup.inc"

/**
 * Reimplements 0x10001010: ZLocGetID.
 * Purpose: Return the retail message-table id for a generated message symbol
 * name, or zero when the symbol is absent from the lookup table.
 */
unsigned int __cdecl ZLocGetID(const char *name)
{
    unsigned int index;
    MessagesLookupRow *row;

    index = 0;
    row = g_MessagesLookupRows;
    while (row < g_MessagesLookupRows + MESSAGES_LOOKUP_ROW_COUNT) {
        if (strcmp(name, row->name) == 0) {
            return g_MessagesLookupRows[index].id;
        }
        ++row;
        ++index;
    }

    return 0;
}
