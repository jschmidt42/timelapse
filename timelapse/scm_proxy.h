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
        string_t dateold{};
        string_t merged_date{};
        string_t description{};

        string_t patch{};
        string_t base_summary{};

        string_t* annotations{};
    };

    bool revision_initialize(revision_t& r, string_const_t* infos, size_t info_count);
    void revision_deallocate(revision_t& rev);

    struct annotations_t
    {
        int revid{};
        string_t file{};
        string_t date{};
        string_t base_summary{};
        string_t* lines{};
        string_t patch{};
    };

    void annotations_initialize(annotations_t& ann);
    void annotations_finailze(annotations_t& ann);

    /// Fetch scm revision for a given file in another thread.
    request_t fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges);

    /// Check if the scm command has finished.
    bool is_request_done(request_t request);

    /// Release any resources used by the fetch command.
    size_t dispose_request(request_t request);

    /// Returns the request result if done.
    const string_t* request_results(request_t request);

    /// Parse the fetch revisions output and return the revision list container.
    generics::vector<revision_t> revision_list(const string_t* changes, size_t change_count);

    /// Fetch additional info for a single revision
    request_t fetch_revision_annotations(const char* file_path, const char* working_dir, int revid);

    /// Parse the annotations command result and return an object with the data.
    annotations_t revision_annotations(request_t request);

}}
