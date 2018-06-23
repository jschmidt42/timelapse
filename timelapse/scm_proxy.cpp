#include "scm_proxy.h"

#include "foundation/windows.h"
#undef THREAD_PRIORITY_HIGHEST

#include "foundation/environment.h"
#include "foundation/string.h"
#include "foundation/thread.h"
#include "foundation/memory.h"
#include "foundation/hash.h"
#include "foundation/beacon.h"

#include <algorithm> // REMOVE ME
#include <vector>// REMOVE ME

#define SCM_ARRAYSIZE(_ARR) ((size_t)(sizeof(_ARR)/sizeof(*(_ARR))))
#define HASH_SCM (static_hash_string("scm", 3, 3754008690416994104ULL))

struct command_t
{
    int context;
    string_t line;
    string_t file;
    string_t dir;

    thread_t* thread;

    string_t result;
};

static void* execute_request(void *arg)
{
    command_t* cmd = (command_t*)arg;
    std::string output = timelapse::scm::execute_command(cmd->line.str, cmd->dir.str);
    cmd->result = string_clone(output.c_str(), output.size());
    return 0;
}

std::string timelapse::scm::execute_command(const char* cmd, const char* working_directory)
{
    std::string strResult;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES), NULL, FALSE };
    saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe to get results from child's stdout.
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
        return strResult;

    STARTUPINFOA si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

    PROCESS_INFORMATION pi = { nullptr, nullptr, 0, 0 };

    const BOOL fSuccess = CreateProcessA(nullptr, (LPSTR)cmd, nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE, nullptr,
        working_directory ? working_directory : environment_current_working_directory().str, &si, &pi);
    if (!fSuccess)
    {
        CloseHandle(hPipeWrite);
        CloseHandle(hPipeRead);
        return strResult;
    }

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

            if (!::ReadFile(hPipeRead, buf, (DWORD)std::min(sizeof(buf) - 1, (size_t)dwAvail), &dwRead, nullptr) || !dwRead)
                // error, the child process might ended
                break;

            buf[dwRead] = 0;
            strResult += buf;
        }
    }

    CloseHandle(hPipeWrite);
    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return strResult;
}

timelapse::scm::request_t timelapse::scm::fetch_revisions(const char* file_path, const char* working_dir, bool wants_merges)
{
    command_t* cmd = (command_t*)memory_allocate(HASH_SCM, sizeof command_t, 0, 0);
    cmd->context = 0;
    cmd->file = string_clone(file_path, strlen(file_path));
    cmd->line = string_allocate_format(STRING_CONST(
        "hg log --template \"{rev}|{author|user}|{node|short}|{date|shortdate}|{desc|strip|firstline}\\n\" " \
        "-r \"ancestors(branch(.))\" %s \"%s\""), wants_merges ? "" : "--no-merges", file_path);
    cmd->dir = string_clone(working_dir, strlen(working_dir));

    cmd->thread = thread_allocate(execute_request, cmd, STRING_CONST("scm request"), THREAD_PRIORITY_HIGHEST, 0);
    thread_start(cmd->thread);

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

    thread_deallocate(cmd->thread);
    string_deallocate(cmd->line.str);
    string_deallocate(cmd->dir.str);
    string_deallocate(cmd->file.str);
    string_deallocate(cmd->result.str);
    memory_deallocate(cmd);

    return 0;
}

string_t timelapse::scm::request_result(request_t request)
{
    if (!is_request_done(request))
        return string_t{ 0, 0 };

    command_t* cmd = (command_t*)request;
    return string_clone(STRING_ARGS(cmd->result));
}

std::vector<timelapse::scm::revision_t> timelapse::scm::revision_list(const string_t& result, const std::vector<timelapse::scm::revision_t>& previous_revisions)
{
    string_const_t changes[100];
    size_t change_count = string_explode(STRING_ARGS(result), STRING_CONST("\n"), changes, SCM_ARRAYSIZE(changes), false);

    std::vector<revision_t> revisions;

    for (size_t i = 0; i < change_count; ++i)
    {
        string_const_t infos[16];
        string_explode(STRING_ARGS(changes[i]), STRING_CONST("|"), infos, SCM_ARRAYSIZE(infos), false);

        revision_t r;
        r.id = string_to_int(STRING_ARGS(infos[0]));
        r.author.assign(infos[1].str, infos[1].length);
        r.rev.assign(infos[2].str, infos[2].length);
        r.date.assign(infos[3].str, infos[3].length);
        r.description.assign(infos[4].str, infos[4].length);

        const auto& fit = std::find_if(previous_revisions.begin(), previous_revisions.end(), [&r](const revision_t& o) { return r.id == o.id; });
        if (fit != previous_revisions.end())
        {
            r.annotations = fit->annotations;
        }

        revisions.push_back(r);
    }

    std::sort(revisions.begin(), revisions.end(), [](const revision_t& a, const revision_t& b) { return a.id < b.id; });

    return revisions;
}

timelapse::scm::request_t timelapse::scm::fetch_revision_annotations(const char* file_path, const char* working_dir, int revid)
{
    command_t* cmd = (command_t*)memory_allocate(HASH_SCM, sizeof command_t, 0, 0);
    cmd->context = revid;
    cmd->file = string_clone(file_path, strlen(file_path));
    cmd->line = string_allocate_format(STRING_CONST("hg annotate --user -c -w -b -B -r %d \"%s\""), revid, file_path);
    cmd->dir = string_clone(working_dir, strlen(working_dir));

    cmd->thread = thread_allocate(execute_request, cmd, STRING_CONST("scm annotate"), THREAD_PRIORITY_HIGHEST, 0);
    thread_start(cmd->thread);

    return (request_t)cmd;
}

timelapse::scm::annotations_t timelapse::scm::revision_annotations(request_t request)
{
    annotations_t ann;
    ann.revid = 0;
    ann.file = "";
    ann.source = "";

    if (!is_request_done(request))
        return ann;

    command_t* cmd = (command_t*)request;

    ann.revid = cmd->context;
    ann.file.assign(cmd->file.str, cmd->file.length);
    ann.source.assign(cmd->result.str, cmd->result.length);
    
    return ann;
}
