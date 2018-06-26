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

    int revision_cursor();
    void set_revision_cursor(int index);

    scm::revision_t* find_revision(int id);
    int set_current_revision(int id);
    scm::revision_t* current_revision();
    const generics::vector<scm::revision_t>& revisions();
    int is_fetching_annotations();
    string_const_t rev_node();

}}
