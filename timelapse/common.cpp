#include "common.h"

#include "foundation/string.h"

size_t string_occurence(const char* str, size_t len, char c)
{
    size_t occurence = 0;
    size_t offset = 0;
    while (true)
    {
        size_t foffset = string_find(str, len, c, offset);
        if (foffset == STRING_NPOS)
            break;
        offset = foffset + 1;
        occurence++;
    }
    return occurence + (offset+1 < len);
}

size_t string_line_count(const char* str, size_t len)
{
    if (!str || len == 0)
        return 0;
    return string_occurence(str, len, STRING_NEWLINE[0])+1;
}
