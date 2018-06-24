#include "common.h"

#include "foundation/string.h"
#include "foundation/hash.h"

#define HASH_COMMON (static_hash_string("common", 6, 14370257353172364778ULL))

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
    return string_occurence(str, len, STRING_NEWLINE[0]);
}

lines_t string_split_lines(const char* str, size_t len)
{
    lines_t lines;
    const size_t line_occurence = string_line_count(str, len);
    lines.items = (string_const_t*)memory_allocate(HASH_COMMON, line_occurence * sizeof string_const_t, 0, 0);
    lines.count = string_explode(str, len, STRING_CONST(STRING_NEWLINE), lines.items, line_occurence, false);
    FOUNDATION_ASSERT(lines.count == line_occurence);
    return lines;
}

void string_lines_finalize(lines_t& lines)
{
    memory_deallocate(lines.items);
    lines.count = 0;
}
