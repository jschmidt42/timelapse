#include "timelapse.h"
#include "scm_proxy.h"
#include "session.h"
#include "scoped_string.h"

#include <foundation/string.h>
#include <foundation/log.h>
#include <foundation/process.h>
#include <foundation/hashstrings.h>
#include "foundation/environment.h"
#include "foundation/array.h"
#include "foundation/foundation.h"

#include <imgui/imgui.h>

#include <vector>// REMOVE ME
#include <sstream>// REMOVE ME

#define CTEXT(str) string_const(STRING_CONST(str))

namespace timelapse {

const char* APP_TITLE = "Timelapse";

scm::request_t g_fetch_revision_request = 0;
std::vector<scm::revision_t> g_revisions;

template<typename Out>
static void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

bool setup(const char* file_path)
{
    session::setup(file_path);
    if (!session::is_valid())
        return false;

    g_fetch_revision_request = scm::fetch_revisions(session::file_path(), session::working_dir(), false, 50);
    return g_fetch_revision_request != 0;
}

int initialize(GLFWwindow* window)
{
    FOUNDATION_UNUSED(window);

    const string_const_t* cmdline = environment_command_line();
    for (int i = 1, end = array_size(cmdline); i < end; ++i)
    {
        string_const_t arg = cmdline[i];
        if (fs_is_file(STRING_ARGS(arg)))
        {
            if (setup(arg.str))
                break;
        }
    }
    
    return 0;
}

void render_main_menu()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File", true))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O", nullptr))
            {
                // TODO: open file dialog selector
            }

            ImGui::Separator();

            if (ImGui::MenuItem("Exit", "Alt+F4", nullptr))
                process_exit(ERROR_NONE);
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

static void render_waiting()
{
    ImGui::PushItemWidth(-1);
    ImGui::Text("Fetching revisions...");
}

static void render_main_window(const char* window_title, float window_width, float window_height)
{
    ImGui::SetWindowPos(window_title, ImVec2(0, 0));
    ImGui::SetWindowSize(window_title, ImVec2(window_width, window_height));

    render_main_menu();

    if (!scm::is_request_done(g_fetch_revision_request))
    {
        render_waiting();
        return;
    }
    
    if (g_fetch_revision_request)
    {
        scoped_string_t result = scm::request_result(g_fetch_revision_request);
        g_revisions = scm::revision_list(result);
        g_fetch_revision_request = scm::dispose_request(g_fetch_revision_request);
    }

    static int revision = (int)g_revisions.size() - 1;

    ImGui::PushItemWidth(-1);
    ImGui::Text("Revision: %s - %s", session::file_path(), revision >= 0 ? g_revisions[revision].rev.c_str() :
        "Nothing to timelapse. Select a file or call this program with the file path as the first argument.");

    if (!g_revisions.empty())
    {
        scm::revision_t& r = g_revisions[revision];
        ImGui::SliderInt("", &revision, 0, (int)g_revisions.size() - 1);

        if (r.source.empty())
        {
            std::string cmd = "hg annotate --user -c -w -b -B -r " + r.rev + " \"" + session::file_path() + "\"";
            r.source = scm::execute_command(cmd.c_str(), session::working_dir());
        }

        ImGui::BeginChild("CodeView", ImVec2(ImGui::GetWindowContentRegionWidth(), 0), true, ImGuiWindowFlags_HorizontalScrollbar);

        //ImGui::InputTextMultiline("", (char*)r.source.c_str(), r.source.size(), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);

        auto lines = split(r.source, '\n');
        for (int i = 0; i < lines.size(); i++)
        {
            const auto pos = lines[i].find(r.rev);
            if (pos != std::string::npos)
                ImGui::TextColored(ImColor(0.6f, 1.0f, 0.6f), "%04d: %s", i, lines[i].c_str());
            else
                ImGui::Text("%04d: %s", i, lines[i].c_str());
        }
        ImGui::EndChild();
    }

    ImGui::PopItemWidth();
}

void render(int window_width, int window_height)
{
    const char* k_MainWindowTitle = APP_TITLE;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
        
    if (ImGui::Begin(k_MainWindowTitle, nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar))
        render_main_window(k_MainWindowTitle, (float)window_width, (float)window_height);

    ImGui::End();
}

void shutdown()
{
    session::shutdown();
}

}

extern void app_exception_handler(const char* dump_file, size_t length)
{
    FOUNDATION_UNUSED(dump_file);
    FOUNDATION_UNUSED(length);
    log_error(HASH_TEST, ERROR_EXCEPTION, STRING_CONST("Unhandled exception"));
    process_exit(-1);
}

extern void app_configure(foundation_config_t& config, application_t& application)
{
    application.name = CTEXT(timelapse::APP_TITLE);
    application.short_name = CTEXT(timelapse::APP_TITLE);
    application.company = CTEXT("Bazesys");
    application.version = string_to_version(STRING_CONST("1.0.0"));
    application.flags = APPLICATION_STANDARD;
    application.exception_handler = app_exception_handler;
}

extern int app_initialize(GLFWwindow* window)
{
    return timelapse::initialize(window);
}

extern void app_render(int display_w, int display_h)
{
    timelapse::render(display_w, display_h);
}

extern void app_shutdown()
{
    timelapse::shutdown();
}

extern const char* app_title()
{
    return timelapse::APP_TITLE;
}