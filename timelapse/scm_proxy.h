#pragma once

#include "foundation/string.h"

#include <string>// REMOVE ME
#include <vector>// REMOVE ME

namespace timelapse { namespace scm {
    
typedef size_t request_t;

struct revision_t
{
    int id{};

    std::string rev{};
    std::string author{};
    std::string branch{};
    std::string date{};
    std::string rawdate{};
    std::string description{};

    std::string annotations{};
};

struct annotations_t
{
    int revid{};
    std::string file{};
    std::string source{};
};

std::string execute_command(const char* cmd, const char* working_directory);

/// Fetch scm revision for a given file in another thread.
request_t fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges);

/// Check if the scm command has finished.
bool is_request_done(request_t request);

/// Release any resources used by the fetch command.
size_t dispose_request(request_t request);

/// Returns the request result if done.
string_t request_result(request_t request);

/// Parse the fetch revisions output and return the revision list container.
std::vector<revision_t> revision_list(const string_t& result, const std::vector<timelapse::scm::revision_t>& previous_revisions);

/// Fetch additional info for a single revision
request_t fetch_revision_annotations(const char* file_path, const char* working_dir, int revid);

/// Parse the annotations command result and return an object with the data.
annotations_t revision_annotations(request_t request);
}}
