#pragma once

#include "scm_proxy.h"

namespace timelapse { namespace session {
  
    void setup(const char* file_path = nullptr);
    void shutdown();

    bool is_valid();
    const char* working_dir();
    const char* file_path();
    bool fetch_revisions();
    bool is_fetching_revisions();
    bool has_revisions();
    void update();
    size_t revision_curosr();
    size_t set_revision_cursor(size_t revision);
    const generics::vector<scm::revision_t>& revisions();
    int is_fetching_annotations();

}}
