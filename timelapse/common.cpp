#include "common.h"

#include "foundation/string.h"

size_t string_occurence(const char* str, size_t len, char c)
{
    size_t occurence = 0;
    size_t offset = 0;
    while (true)
    {
        offset = string_find(str, len, c, offset);
        if (offset == STRING_NPOS)
            break;
        offset++;
        occurence++;
    }
    return occurence;
}
