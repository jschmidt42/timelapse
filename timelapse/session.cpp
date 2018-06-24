#include "session.h"
#include "scm_proxy.h"
#include "scoped_string.h"
#include "common.h"

#include "foundation/environment.h"
#include "foundation/path.h"
#include "foundation/string.h"
#include "foundation/foundation.h"

namespace timelapse { namespace session {

// File being timelapsed
string_t g_file_path{};
string_t g_working_dir{};

// SCM request tokens
scm::request_t g_request_fetch_revisions = 0;
scm::request_t g_request_fetch_single_revision = 0;

// Fetch revision data
size_t g_revision_cursor = -1;
generics::vector<scm::revision_t> g_revisions;
int g_last_fetched_revid_annotations = 0;

void cleanup()
{
    string_deallocate(g_file_path.str);
    string_deallocate(g_working_dir.str);
}

static void cancel_pending_requests()
{
    if (g_request_fetch_revisions != 0)
        g_request_fetch_revisions = scm::dispose_request(g_request_fetch_revisions);

    if (g_request_fetch_single_revision != 0)
        g_request_fetch_single_revision = scm::dispose_request(g_request_fetch_single_revision);
}

static void clear_revisions_info()
{
    for (auto& rev: g_revisions)
        scm::revision_deallocate(rev);
    g_revisions.clear();
    g_revision_cursor = -1;
    g_last_fetched_revid_annotations = 0;
}

void setup(const char* file_path)
{
    // setup can be called multiple times, so cleaning up first.
    cleanup();

    if (file_path)
    {
        g_file_path = string_clone(file_path, strlen(file_path));
        g_working_dir = string_clone_string(path_directory_name(STRING_ARGS(g_file_path)));
    }
    else
    {
        g_file_path = string_clone_string(string_empty());
        g_working_dir =  string_clone_string(environment_current_working_directory());
    }

    if (is_valid())
    {
        cancel_pending_requests();
        clear_revisions_info();
    }
}

void shutdown()
{
    cancel_pending_requests();
    clear_revisions_info();
    cleanup();
}

bool fetch_revisions()
{
    g_request_fetch_revisions = scm::fetch_revisions(file_path(), working_dir(), false);
    return g_request_fetch_revisions != 0;
}

bool has_revisions()
{
    return g_revisions.size() > 0;
}

bool is_fetching_revisions()
{
    return !scm::is_request_done(g_request_fetch_revisions);
}

void update()
{
    if (g_request_fetch_revisions != 0)
    {
        // Check fetched revisions
        if (scm::is_request_done(g_request_fetch_revisions))
        {
            size_t revision_count_before = g_revisions.size();

            scoped_string_t result = scm::request_result(g_request_fetch_revisions);
            g_revisions = scm::revision_list(result);
            g_request_fetch_revisions = scm::dispose_request(g_request_fetch_revisions);

            set_revision_cursor((g_revision_cursor + 1) + (g_revisions.size() - revision_count_before) - 1);
        }
    }

    if (g_request_fetch_single_revision == 0 && !g_revisions.empty())
    {
        if (g_revision_cursor < g_revisions.size())
        {
            const auto& crev = g_revisions[g_revision_cursor];
            if (crev.annotations.length == 0)
            {
                g_last_fetched_revid_annotations = crev.id;
                g_request_fetch_single_revision = scm::fetch_revision_annotations(file_path(), working_dir(), crev.id);
            }
        }

        if (g_request_fetch_single_revision == 0)
        {
            // Fetch extra info for individual revisions
            for (size_t i = g_revisions.size()-1; i != -1; --i)
            {
                auto& rev = g_revisions[i];
                if (rev.annotations.length == 0)
                {
                    g_last_fetched_revid_annotations = rev.id;
                    g_request_fetch_single_revision = scm::fetch_revision_annotations(file_path(), working_dir(), rev.id);
                    break;
                }
            }
        }
    }
    else if (g_request_fetch_single_revision != 0 && scm::is_request_done(g_request_fetch_single_revision))
    {
        scm::annotations_t annotations = scm::revision_annotations(g_request_fetch_single_revision);
        for(auto& rev: g_revisions)
        {
            if (annotations.revid == rev.id)
            {
                rev.annotations = string_clone(STRING_ARGS(annotations.source));
                break;
            }
        }
        string_deallocate(annotations.file.str);
        string_deallocate(annotations.source.str);
        g_request_fetch_single_revision = scm::dispose_request(g_request_fetch_single_revision);
    }
}

size_t revision_curosr()
{
    return g_revision_cursor;
}

const generics::vector<scm::revision_t>& revisions()
{
    return g_revisions;
}

int is_fetching_annotations()
{
    if (scm::is_request_done(g_request_fetch_single_revision))
        return 0;

    return g_last_fetched_revid_annotations;
}

string_const_t rev_node()
{
    if (g_revisions.empty())
        return string_empty();
    return string_to_const(g_revisions[g_revision_cursor].rev);
}

size_t set_revision_cursor(size_t revision)
{
    if (!g_revisions.empty())
        g_revision_cursor = generics::max(0ULL, generics::min((size_t)revision, g_revisions.size()-1ULL));
    else
        g_revision_cursor = -1;
    return g_revision_cursor;
}

bool is_valid()
{
    return fs_is_file(STRING_ARGS(g_file_path));
}

const char* file_path()
{
    if (is_valid())
        return g_file_path.str;
    return "";
}

const char* working_dir()
{
    if (is_valid())
        return g_working_dir.str;
    return environment_current_working_directory().str;
}

}}
