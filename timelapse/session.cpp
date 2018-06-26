#include "session.h"
#include "scm_proxy.h"
#include "common.h"

#include "foundation/environment.h"
#include "foundation/path.h"
#include "foundation/string.h"
#include "foundation/foundation.h"

#include <algorithm>

namespace timelapse { namespace session {

// File being timelapsed
string_t g_file_path{};
string_t g_working_dir{};

// SCM request tokens
const size_t MAX_SINGLE_FETCH = 3;
scm::request_t g_request_fetch_revisions = 0;
scm::request_t g_request_fetch_single_revisions[MAX_SINGLE_FETCH] = { 0 };

// Fetch revision data
int g_current_revision_id = -1;
generics::vector<scm::revision_t> g_revisions;

static void cleanup()
{
    string_deallocate(g_file_path.str);
    string_deallocate(g_working_dir.str);
}

static void cancel_pending_requests()
{
    if (g_request_fetch_revisions != 0)
        g_request_fetch_revisions = scm::dispose_request(g_request_fetch_revisions);

    for (size_t  i = 0; i < MAX_SINGLE_FETCH; ++i)
    {
        if (g_request_fetch_single_revisions[i] != 0)
            g_request_fetch_single_revisions[i] = scm::dispose_request(g_request_fetch_single_revisions[i]);
    }
}

static void clear_revisions_info()
{
    for (auto& rev: g_revisions)
        scm::revision_deallocate(rev);
    g_revisions.clear();
    g_current_revision_id = -1;
}

static bool revision_compare(const scm::revision_t& a, const scm::revision_t& b)
{
    if (a.merged_date.length == 0 && b.merged_date.length != 0)
        return false;
    if (a.merged_date.length != 0 && b.merged_date.length == 0)
        return true;
    if (a.merged_date.length == 0 && b.merged_date.length == 0)
        return strcmp(a.date.str, b.date.str) < 0;
    return strcmp(a.merged_date.str, b.merged_date.str) < 0;
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
    // TODO: throttle update

    if (g_request_fetch_revisions != 0)
    {
        // Check fetched revisions
        if (scm::is_request_done(g_request_fetch_revisions))
        {
            const string_t* results = scm::request_results(g_request_fetch_revisions);
            g_revisions = scm::revision_list(results, array_size(results));
            g_request_fetch_revisions = scm::dispose_request(g_request_fetch_revisions);

            std::sort(g_revisions.begin(), g_revisions.end(), revision_compare);

            if (g_revisions.size() > 0)
                set_current_revision(g_revisions.back().id);
        }
    }
    
    if (!g_revisions.empty())
    {
        for (size_t  i = 0; i < MAX_SINGLE_FETCH; ++i)
        {
            if (g_request_fetch_single_revisions[i] == 0)
            {
                int fetch_new_revision_id = -1;

                // Make sure we have annotations data for the current revision
                scm::revision_t* crev = current_revision();
                if (crev && array_size(crev->annotations) == 0)
                {
                    fetch_new_revision_id = crev->id;
                    array_push(crev->annotations, string_clone(STRING_CONST("Fetching data...")));
                }
                else
                {
                    // Otherwise fetch extra info for individual revisions
                    for (size_t i = g_revisions.size() - 1; i != -1; --i)
                    {
                        auto& rev = g_revisions[i];
                        if (array_size(rev.annotations) == 0)
                        {
                            fetch_new_revision_id = rev.id;
                            array_push(rev.annotations, string_clone(STRING_CONST("Fetching data...")));
                            break;
                        }
                    }
                }

                if (fetch_new_revision_id != -1)
                {
                    g_request_fetch_single_revisions[i] = scm::fetch_revision_annotations(file_path(), working_dir(), fetch_new_revision_id);
                }
            }
            else if (g_request_fetch_single_revisions[i] != 0 && scm::is_request_done(g_request_fetch_single_revisions[i]))
            {
                scm::annotations_t annotations = scm::revision_annotations(g_request_fetch_single_revisions[i]);
                g_request_fetch_single_revisions[i] = scm::dispose_request(g_request_fetch_single_revisions[i]);

                scm::revision_t* rev = find_revision(annotations.revid);
                if (rev)
                {
                    std::swap(rev->patch, annotations.patch);
                    std::swap(rev->base_summary, annotations.base_summary);
                    std::swap(rev->merged_date, annotations.date);
                    std::swap(rev->annotations, annotations.lines);
                    FOUNDATION_ASSERT(array_size(rev->annotations) != 0);

                    std::sort(g_revisions.begin(), g_revisions.end(), revision_compare);
                }

                scm::annotations_finailze(annotations);
            }
        }
    }
}

const generics::vector<scm::revision_t>& revisions()
{
    return g_revisions;
}

bool is_fetching_annotations()
{
    for (size_t i = 0; i < MAX_SINGLE_FETCH; ++i)
    {
        if (!scm::is_request_done(g_request_fetch_single_revisions[i]))
            return true;
    }
    return false;
}

string_const_t rev_node()
{
    scm::revision_t* crev = current_revision();
    if (!crev)
        return string_empty();
    return string_to_const(crev->rev);
}

int revision_cursor()
{
    if (g_current_revision_id <= 0)
        return -1;

    for (size_t i = 0, end = (int)g_revisions.size(); i != end; ++i)
    {
        if (g_revisions[i].id == g_current_revision_id)
            return (int)i;
    }

    return -1;
}

void set_revision_cursor(int index)
{
    const int revision_count = (int)g_revisions.size();
    if (index >= 0)
        index = index % revision_count;
    else
        index = revision_count + index;
    if (index >= 0 && index < revision_count)
        set_current_revision(g_revisions[index].id);
}

scm::revision_t* find_revision(int id)
{
    for (auto& rev: g_revisions)
        if (rev.id == id)
            return &rev;
    return nullptr;
}

int set_current_revision(int id)
{
    for (auto& rev : g_revisions)
    {
        if (rev.id == id)
        {
            g_current_revision_id = id;
            return id;
        }
    }
    g_current_revision_id = -1;
    return g_current_revision_id;
}

scm::revision_t* current_revision()
{
    return find_revision(g_current_revision_id);
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
