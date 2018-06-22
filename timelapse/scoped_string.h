#pragma once

#include "foundation/string.h"

struct scoped_string_t
{
    scoped_string_t(const char* s)
        : value(string_clone(s, strlen(s)))
    {
    }

    scoped_string_t(string_t&& o)
    {
        value.str = o.str;
        value.length = o.length;
        o.str = nullptr;
        o.length = 0;
    }

    scoped_string_t(const scoped_string_t& other)
    : value(string_clone(other.value.str, other.value.length))
    {
    }

    scoped_string_t(scoped_string_t&& o) noexcept 
    {
        value.str = o.value.str;
        value.length = o.value.length;
        o.value.str = nullptr;
        o.value.length = 0;
    }

    ~scoped_string_t()
    {
        string_deallocate(value.str);
    }

    operator string_t&()
    {
        return value;
    }

    operator const char*()
    {
        return value.str;
    }

    size_t length() const
    {
        return value.length;
    }

    string_t value{};
};
