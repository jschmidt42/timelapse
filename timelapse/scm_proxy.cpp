#include "scm_proxy.h"
#include "scoped_string.h"
#include "common.h"

#include "foundation/windows.h"
#undef THREAD_PRIORITY_HIGHEST

#include "foundation/environment.h"
#include "foundation/string.h"
#include "foundation/thread.h"
#include "foundation/memory.h"
#include "foundation/hash.h"
#include "foundation/beacon.h"
#include "foundation/array.h"

#define SCM_ARRAYSIZE(_ARR) ((size_t)(sizeof(_ARR)/sizeof(*(_ARR))))
#define HASH_SCM (static_hash_string("scm", 3, 3754008690416994104ULL))

struct command_t
{
    int context{};
    string_t line{};
    string_t file{};
    string_t dir{};

    thread_t* thread{};

    string_t* results{};
};

static command_t* command_allocate(int id, const char* file_path, const char* working_dir, thread_fn fn)
{
    command_t* cmd = (command_t*)memory_allocate(HASH_SCM, sizeof command_t, 0, 0);
    cmd->context = id;
    
    cmd->line = { 0,0 };
    cmd->file = string_clone(file_path, strlen(file_path));
    cmd->dir = string_clone(working_dir, strlen(working_dir));

    cmd->thread = thread_allocate(fn, cmd, STRING_CONST("scm command"), THREAD_PRIORITY_HIGHEST, 0);
    cmd->results = nullptr;

    return cmd;
}

static bool command_execute(command_t* cmd, const char* format, size_t length, ...)
{
    if (format)
    {
        va_list list;
        va_start(list, length);
        cmd->line = string_allocate_vformat(format, length, list);
        va_end(list);
    }

    return thread_start(cmd->thread);
}

static void command_deallocate(command_t* cmd)
{
    thread_deallocate(cmd->thread);

    string_deallocate(cmd->line.str);
    string_deallocate(cmd->dir.str);
    string_deallocate(cmd->file.str);

    if (cmd->results)
    {
        string_array_deallocate(cmd->results);
    }

    memory_deallocate(cmd);
}

static string_t execute_command(const char* cmd, const char* working_directory)
{
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe to get results from child's stdout.
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
        return {0, 0};

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = { nullptr, nullptr, 0, 0 };

    const BOOL fSuccess = CreateProcessA(nullptr, (LPSTR)cmd, nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE, nullptr,
        working_directory ? working_directory : environment_current_working_directory().str, &si, &pi);
    if (!fSuccess)
    {
        CloseHandle(hPipeWrite);
        CloseHandle(hPipeRead);
        return {0, 0};
    }

    string_t strResult = string_allocate(0, 1024);
    bool bProcessEnded = false;
    for (; !bProcessEnded;)
    {
        // Give some timeslice (50ms), so we won't waste 100% cpu.
        bProcessEnded = WaitForSingleObject(pi.hProcess, 50) == WAIT_OBJECT_0;

        // Even if process exited - we continue reading, if there is some data available over pipe.
        for (;;)
        {
            char buf[1024];
            DWORD dwRead = 0;
            DWORD dwAvail = 0;

            if (!::PeekNamedPipe(hPipeRead, nullptr, 0, nullptr, &dwAvail, nullptr))
                break;

            if (!dwAvail) // no data available, return
                break;

            if (!::ReadFile(hPipeRead, buf, (DWORD)generics::min(sizeof(buf) - 1, (size_t)dwAvail), &dwRead, nullptr) || !dwRead)
                // error, the child process might ended
                break;

            buf[dwRead] = 0;

            string_t temp = strResult;
            strResult = string_allocate_concat(STRING_ARGS(strResult), buf, dwRead-1);
            string_deallocate(temp.str);
        }
    }

    CloseHandle(hPipeWrite);
    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return strResult;
}

static void* execute_cmd_line_request(void *arg)
{
    command_t* cmd = (command_t*)arg;
    scoped_string_t output = execute_command(cmd->line.str, cmd->dir.str);
    
    lines_t lines = string_split_lines(STRING_ARGS(output.value));
    for (size_t i = 0; i < lines.count; ++i)
        array_push(cmd->results, string_clone(STRING_ARGS(lines[i])));
    string_lines_finalize(lines);

    return 0;
}

static void* execute_annotations_request(void *arg)
{
    command_t* cmd = (command_t*)arg;

    // TODO: Add option to ignore whitespaces
    // TODO annotate -d -q to have short dates
    
    scoped_string_t annotate = string_allocate_format(STRING_CONST("hg annotate --user -a -c -r %d \"%s\""), cmd->context, cmd->file.str);
    scoped_string_t output = execute_command(annotate, cmd->dir.str);
    array_push(cmd->results, string_clone(STRING_ARGS(output.value)));

    scoped_string_t extras = string_allocate_format(STRING_CONST("hg log --template \"{date}\" -r \"min(descendants(%d) and branch(.))\""), cmd->context);
    output = execute_command(extras, cmd->dir.str);
    array_push(cmd->results, string_clone(STRING_ARGS(output.value)));

    scoped_string_t patch = string_allocate_format(STRING_CONST("hg diff -c %d -- \"%s\""), cmd->context, cmd->file.str);
    output = execute_command(patch, cmd->dir.str);
    array_push(cmd->results, string_clone(STRING_ARGS(output.value)));

    return 0;
}

void timelapse::scm::revision_initialize(revision_t& r, string_const_t* infos)
{
    r.id = string_to_int(STRING_ARGS(infos[0]));
    r.author = string_clone_string(infos[1]);
    r.rev = string_clone_string(infos[2]);
    r.date = string_clone_string(infos[3]);
    r.rawdate = string_clone_string(infos[4]);
    r.branch = string_clone_string(infos[5]);
    r.description = string_clone_string(infos[6]);

    r.patch = {0,0};
    r.annotations = nullptr;
}

void timelapse::scm::revision_deallocate(revision_t& rev)
{
    string_deallocate(rev.rev.str);
    string_deallocate(rev.author.str);
    string_deallocate(rev.branch.str);
    string_deallocate(rev.date.str);
    string_deallocate(rev.rawdate.str);
    string_deallocate(rev.description.str);
    string_deallocate(rev.patch.str);

    string_array_deallocate(rev.annotations);
}

void timelapse::scm::annotations_initialize(annotations_t& ann)
{
    ann.revid = 0;
    ann.file = { 0, 0 };
    ann.date = { 0, 0 };
    ann.patch = { 0, 0 };
    ann.lines = nullptr;
}

void timelapse::scm::annotations_finailze(annotations_t& ann)
{
    string_deallocate(ann.file.str);
    string_deallocate(ann.date.str);
    string_deallocate(ann.patch.str);

    string_array_deallocate(ann.lines);
}

timelapse::scm::request_t timelapse::scm::fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges)
{
    command_t* cmd = command_allocate(0, file_path, working_dir, execute_cmd_line_request);

    command_execute(cmd, STRING_CONST(
        "hg log --template \"{rev}|{author|user}|{node|short}|{date|shortdate}|{date}|{branch}|{desc|strip|firstline}\\n\" " \
        "-r \"ancestors(branch(.))\" %s \"%s\""), wants_merges ? "" : "--no-merges", file_path);
    
    return (request_t)cmd;
}

bool timelapse::scm::is_request_done(request_t request)
{
    if (request == 0)
        return true;

    command_t* cmd = (command_t*)request;
    return !thread_is_running(cmd->thread);
}

size_t timelapse::scm::dispose_request(request_t request)
{
    command_t* cmd = (command_t*)request;

    if (thread_is_running(cmd->thread))
        beacon_try_wait(&cmd->thread->beacon, 500);

    command_deallocate(cmd);
    return 0;
}

const string_t* timelapse::scm::request_results(request_t request)
{
    if (!is_request_done(request))
        return nullptr;

    const auto cmd = (command_t*)request;
    return cmd->results;
}

generics::vector<timelapse::scm::revision_t> timelapse::scm::revision_list(const string_t* changes, size_t change_count)
{
    generics::vector<revision_t> revisions;

    for (size_t i = 0; i < change_count; ++i)
    {
        string_const_t infos[16];
        string_explode(STRING_ARGS(changes[i]), STRING_CONST("|"), infos, SCM_ARRAYSIZE(infos), false);

        revision_t r;
        revision_initialize(r, infos);
        revisions.push_back(r);
    }

    //std::sort(revisions.begin(), revisions.end(), [](const revision_t& a, const revision_t& b) { return strcmp(a.rawdate.str, b.rawdate.str) < 0; });

    return revisions;
}

timelapse::scm::request_t timelapse::scm::fetch_revision_annotations(const char* file_path, const char* working_dir, int revid)
{
    command_t* cmd = command_allocate(revid, file_path, working_dir, execute_annotations_request);
    command_execute(cmd, nullptr, 0);
    return (request_t)cmd;
}

timelapse::scm::annotations_t timelapse::scm::revision_annotations(request_t request)
{
    annotations_t ann;
    annotations_initialize(ann);
    
    if (!is_request_done(request))
        return ann;

    auto* cmd = (command_t*)request;

    ann.revid = cmd->context;

    ann.file = string_clone(STRING_ARGS(cmd->file));
    ann.date = string_clone(STRING_ARGS(cmd->results[1]));
    ann.patch = string_clone(STRING_ARGS(cmd->results[2]));

    lines_t lines = string_split_lines(STRING_ARGS(cmd->results[0]));
    for (size_t i = 0; i < lines.count; ++i)
        array_push(ann.lines, string_clone(STRING_ARGS(lines[i])));
    string_lines_finalize(lines);
    
    return ann;
}
