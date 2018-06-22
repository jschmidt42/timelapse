#pragma once

#include <string>

namespace timelapse { namespace scm {
    
struct revision_t
{
    int id;
    int mods;

    std::string rev;
    std::string author;
    std::string date;
    std::string description;

    std::string source;
};

}}
