#include "timelapse.h"
#include "scm_proxy.h"
#include "session.h"
#include "scoped_string.h"
#include "common.h"

#include "foundation/string.h"
#include "foundation/log.h"
#include "foundation/process.h"
#include "foundation/hashstrings.h"
#include "foundation/environment.h"
#include "foundation/array.h"
#include "foundation/foundation.h"
#include "foundation/windows.h"

#include <Commdlg.h>

#include <imgui/imgui.h>

#include <GLFW/glfw3.h>

#define CTEXT(str) string_const(STRING_CONST(str))
#define HASH_TIMELAPSE (static_hash_string("timelapse", 9, 1468255696394573417ULL))

extern HWND g_WindowHandle;
extern GLFWwindow* g_Window;

namespace timelapse {

const char* APP_TITLE = "Timelapse";

static void set_window_title(GLFWwindow* window, const char* file_path)
{
    scoped_string_t dirpath = path_directory_name(file_path, strlen(file_path));
    scoped_string_t filename = path_file_name(file_path, strlen(file_path));
    scoped_string_t title = string_allocate_format(STRING_CONST("%s [%s] - %s"), (const char*)filename, (const char*)dirpath, APP_TITLE);
    glfwSetWindowTitle(window, title);
}

static bool setup(GLFWwindow* window, const char* file_path)
{
    session::setup(file_path);
    if (!session::is_valid())
        return false;

    set_window_title(window, file_path);
    return session::fetch_revisions();
}

static bool shortcut_executed(bool ctrl, bool alt, bool shift, bool super, int key)
{
    ImGuiIO& io = ImGui::GetIO();
    return ((!ctrl || io.KeyCtrl) && (!alt || io.KeyAlt) && (!shift || io.KeyShift) && (!super || io.KeySuper) &&
        io.KeysDown[key] && io.KeysDownDuration[key] == 0.0f);
}

//static bool shortcut_executed(bool ctrl, bool alt, bool shift, int key) { return shortcut_executed(ctrl, alt, shift, false, key); }
//static bool shortcut_executed(bool ctrl, bool alt, int key) { return shortcut_executed(ctrl, alt, false, false, key); }
static bool shortcut_executed(bool ctrl, int key) { return shortcut_executed(ctrl, false, false, false, key); }
static bool shortcut_executed(int key) { return shortcut_executed(false, false, false, false, key); }

static void open_file()
{
    char file_path[1024] = {'\0'};
    string_format(file_path, sizeof(file_path)-1, STRING_CONST("%s"), session::file_path());
    path_clean(file_path, strlen(file_path), sizeof(file_path)-1);
    string_replace(file_path, strlen(file_path), sizeof(file_path)-1, STRING_CONST("/"), STRING_CONST("\\"), true);
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = g_WindowHandle;
    ofn.lpstrFile = file_path;
    ofn.nMaxFile = sizeof(file_path);
    ofn.lpstrFilter = "All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = (LPSTR)"Select file to timelapse...";
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
    if (GetOpenFileName(&ofn))
    {
        setup(g_Window, ofn.lpstrFile);
    }
}

static void goto_previous_revision()
{
    session::set_revision_cursor(session::revision_curosr()-1);
}

static void goto_next_revision()
{
    session::set_revision_cursor(session::revision_curosr()+1);
}

static void goto_next_change()
{
    if (!session::has_revisions())
        return;

    log_info(0, STRING_CONST("TODO: goto next change..."));
}

static void execute_tool(const string_const_t& name, string_const_t* argv, size_t argc)
{
    process_t* tool = process_allocate();
    process_set_executable_path(tool, STRING_ARGS(name));
    process_set_working_directory(tool, session::working_dir(), strlen(session::working_dir()));
    process_set_arguments(tool, argv, argc);
    process_set_flags(tool, PROCESS_DETACHED | PROCESS_HIDE_WINDOW);
    process_spawn(tool);
    process_deallocate(tool);
}

static void open_compare_tool()
{
    if (!session::has_revisions())
        return;
        
    string_const_t args[3] = {CTEXT("bcomp"), CTEXT("-c"), session::rev_node()};
    execute_tool(CTEXT("hg"), args, 3);
}

static void open_revdetails_tool()
{
    if (!session::has_revisions())
        return;

    string_const_t args[3] = { CTEXT("revdetails"), CTEXT("-r"), session::rev_node() };
    execute_tool(CTEXT("thg"), args, 3);
}

static void render_main_menu()
{
    if (shortcut_executed(true, 'O'))
        open_file();
    else  if (shortcut_executed(GLFW_KEY_PAGE_UP))
        goto_previous_revision();
    else if (shortcut_executed(GLFW_KEY_PAGE_DOWN))
        goto_next_revision();
    else if (shortcut_executed(GLFW_KEY_SPACE))
        goto_next_change();
    else if (shortcut_executed(true, 'D'))
        open_compare_tool();
    else if (shortcut_executed(GLFW_KEY_F1))
        open_revdetails_tool();

    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Open", "Ctrl+O")) open_file();
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) process_exit(ERROR_NONE);
            
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Revision", session::has_revisions()))
        {
            if (ImGui::MenuItem("Previous", "Pg. Down")) goto_previous_revision();
            if (ImGui::MenuItem("Next", "Pg. Up")) goto_next_revision();
            ImGui::Separator();
            if (ImGui::MenuItem("Cycle changes", "Space")) goto_next_change();

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools", session::has_revisions()))
        {
            if (ImGui::MenuItem("Compare", "Ctrl+D")) open_compare_tool();
            if (ImGui::MenuItem("Details", "F1")) open_revdetails_tool();

            ImGui::EndMenu();
        }

//         if (ImGui::BeginMenu("Siblings", false))
//         {
//             // TODO:
//             ImGui::EndMenu();
//         }

        if (int revid = session::is_fetching_annotations())
        {
            char fetch_annotations_label[1024] = {'\0'};
            string_format(STRING_CONST(fetch_annotations_label), STRING_CONST("Fetching annotations for %d..."), revid);
            if (ImGui::BeginMenu(fetch_annotations_label, false))
                ImGui::EndMenu();
        }
        else if (session::is_fetching_revisions())
        {
            if (ImGui::BeginMenu("Fetching revisions...", false))
                ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

static void render_waiting()
{
    ImGui::PushItemWidth(-1);
    ImGui::Text("Building up a time machine for you...");
}

static void render_main_window(const char* window_title, float window_width, float window_height)
{
    ImGui::SetWindowPos(window_title, ImVec2(0, 0));
    ImGui::SetWindowSize(window_title, ImVec2(window_width, window_height));

    render_main_menu();

    if (!session::has_revisions() && session::is_fetching_revisions())
    {
        render_waiting();
        return;
    }
    
    size_t revision = session::revision_curosr();
    bool has_revision_to_show = revision != -1;

    if (!has_revision_to_show)
        ImGui::Text("Nothing to timelapse. Select a file or call this program with the file path as the first argument.");
    
    const auto& revisions = session::revisions();
    if (!revisions.empty())
    {
        const auto& crev = revisions[revision];
        char clog[2048] = { '\0' };
        string_format(STRING_CONST(clog), STRING_CONST("%s | %s | %s | %s"), crev.rev.str, crev.branch.str, crev.author.str, crev.date.str);

        ImGui::PushItemWidth(-1);
        ImGui::BeginGroup();
        ImGui::InputText("summary", clog, sizeof(clog) - 1, ImGuiInputTextFlags_ReadOnly);
        ImGui::TextWrapped(crev.description.str);

        int irev = (int)revision+1;
        if (ImGui::SliderInt("", &irev, 1, (int)revisions.size()))
            session::set_revision_cursor((size_t)irev - 1);
        ImGui::EndGroup();
        ImGui::PopItemWidth();
        
        if (ImGui::BeginChild("CodeView", ImVec2(ImGui::GetWindowContentRegionWidth(), 0), true, ImGuiWindowFlags_HorizontalScrollbar))
        {
            if (!crev.annotations.length == 0)
            {
                //ImGui::InputTextMultiline("", (char*)r.source.c_str(), r.source.size(), ImGui::GetContentRegionAvail(), ImGuiInputTextFlags_ReadOnly);

                const size_t line_occurence = string_occurence(STRING_ARGS(crev.annotations), STRING_NEWLINE[0]);

                string_const_t* lines = (string_const_t*)memory_allocate(HASH_TIMELAPSE, line_occurence * sizeof string_const_t, 0, 0);
                size_t line_count = string_explode(STRING_ARGS(crev.annotations), STRING_CONST("\n"), lines, line_occurence, false);

                for (int i = 0, max_line_digit_count = num_digits(line_count); i < line_count; ++i)
                {
                    const auto pos = string_find_string(STRING_ARGS(lines[i]), STRING_ARGS(crev.rev), 0);
                    if (pos != STRING_NPOS)
                        ImGui::TextColored(ImColor(0.6f, 1.0f, 0.6f), "%0*d: %.*s", max_line_digit_count, i + 1, lines[i].length, lines[i].str);
                    else
                        ImGui::Text("%0*d: %.*s", max_line_digit_count, i + 1, lines[i].length, lines[i].str);
                }

                memory_deallocate(lines);
            }
            else
            {
                ImGui::Text("Fetching annotations, be right back...");
            }

            ImGui::EndChild();
        }
    }
}

int initialize(GLFWwindow* window)
{
    FOUNDATION_UNUSED(window);

    const string_const_t* cmdline = environment_command_line();
    for (int i = 1, end = array_size(cmdline); i < end; ++i)
    {
        string_const_t arg = cmdline[i];
        scoped_string_t file_path = path_allocate_absolute(STRING_ARGS(arg));
        
        if (fs_is_file(STRING_ARGS(file_path.value)))
        {
            if (setup(window, file_path))
                break;
        }
    }

    return 0;
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
    {
        session::update();

        render_main_window(k_MainWindowTitle, (float)window_width, (float)window_height);
    }

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
