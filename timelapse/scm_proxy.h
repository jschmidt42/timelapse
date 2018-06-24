#pragma once

#include "common.h"

namespace timelapse { namespace scm {
    
    typedef size_t request_t;

    struct revision_t
    {
        int id{};

        string_t rev{};
        string_t author{};
        string_t branch{};
        string_t date{};
        string_t rawdate{};
        string_t description{};

        string_t annotations{};
    };

    void revision_initialize(revision_t& rev, string_const_t* infos);
    void revision_deallocate(revision_t& rev);

    struct annotations_t
    {
        int revid{};
        string_t file{};
        string_t source{};
    };

    /// Fetch scm revision for a given file in another thread.
    request_t fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges);

    /// Check if the scm command has finished.
    bool is_request_done(request_t request);

    /// Release any resources used by the fetch command.
    size_t dispose_request(request_t request);

    /// Returns the request result if done.
    string_t request_result(request_t request);

    /// Parse the fetch revisions output and return the revision list container.
    generics::vector<revision_t> revision_list(const string_t& result);

    /// Fetch additional info for a single revision
    request_t fetch_revision_annotations(const char* file_path, const char* working_dir, int revid);

    /// Parse the annotations command result and return an object with the data.
    annotations_t revision_annotations(request_t request);

}}
