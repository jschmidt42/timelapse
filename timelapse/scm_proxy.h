#pragma once

#include "foundation/string.h"

#include <string>// REMOVE ME
#include <vector>// REMOVE ME

namespace timelapse { namespace scm {
    
typedef size_t request_t;

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

std::string execute_command(const char* cmd, const char* working_directory);

/// Fetch scm revision for a given file in another thread.
request_t fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges, int last_count);

/// Check if the scm command has finished.
bool is_request_done(request_t request);

/// Release any resources used by the fetch command.
size_t dispose_request(request_t request);

/// Returns the request result if done.
string_t request_result(request_t request);

std::vector<revision_t> revision_list(const string_t& result);
}}
