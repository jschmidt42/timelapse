#pragma once

#include "scm_proxy.h"

namespace timelapse {
namespace session {
  
    /// Setup the user session around the specified file path
    void setup(const char* file_path = nullptr);

    /// Periodically update the current session data (check scm requests, compile data, etc.)
    void update();

    /// Cleanup any resources used by the current user session
    void shutdown();

    /// Is the session initialized and valid (i.e. any file path being timelapsed?)
    bool is_valid();

    /// Returns the current working directory (dir. path of the current file)
    const char* working_dir();

    /// Returns the current file path being inspected/timelapsed
    const char* file_path();

    /// Fetch current file revisions in another thread
    bool fetch_revisions();

    /// Are the current file revision fetched?
    bool is_fetching_revisions();

    /// Do we have any file revisions available?
    bool has_revisions();

    /// Returns the current revision index in the revision list (i.e. session::revisions())
    int revision_cursor();

    /// Sets the current revision on the specified index
    void set_revision_cursor(int index);

    /// Returns the revision matching the specified id if any, otherwise nullptr is returns (ids are local to the user's computer)
    scm::revision_t* find_revision(int id);

    /// Sets the current revision being watched by id (if any)
    int set_current_revision(int id);

    /// Returns the current revision (if any, otherwise nullptr is returned)
    scm::revision_t* current_revision();

    /// Returns the revision sets of the current file being watched.
    const generics::vector<scm::revision_t>& revisions();

    /// Are we currently fetching some revision data (i.e. annotations, patch, meta-data, etc.)
    int is_fetching_annotations();

    /// Returns the current revision node info if any, otherwise an empty string is returned.
    string_const_t rev_node();

}}
