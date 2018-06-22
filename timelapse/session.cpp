#include "session.h"

#include "foundation/environment.h"
#include "foundation/path.h"
#include "foundation/string.h"
#include "foundation/foundation.h"

namespace timelapse { namespace session {

string_t g_file_path{};
string_t g_working_dir{};

void cleanup()
{
    string_deallocate(g_file_path.str);
    string_deallocate(g_working_dir.str);
}

void setup(const char* file_path)
{
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
}

void shutdown()
{
    cleanup();
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
