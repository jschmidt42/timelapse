#include "stdafx.h"
#include "timelapse.h"

#include <imgui/imgui.h>

#include <vector>
#include <sstream>
#include <algorithm>

namespace tl {

static char s_WorkingDirectory[MAX_PATH] = {'\0'};
static std::string s_FilePath;

struct Revision
{
    int id;
    int mods;

    std::string rev;
    std::string author;
    std::string date;
    std::string description;

    std::string source;
};

std::vector<Revision> s_Revisions;

template<typename Out>
void split(const std::string &s, char delim, Out result) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        *(result++) = item;
    }
}

std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, std::back_inserter(elems));
    return elems;
}

static const char* FindFilePathLastPathSep(const char* file_path, char sep = '\\')
{
    const char *p = strrchr(file_path, sep);
    if (p)
        return p;

    if (sep != '/')
        return FindFilePathLastPathSep(file_path, '/');

    return nullptr;
}

bool setup(const char* filePath)
{
    const char *p = FindFilePathLastPathSep(filePath);
    if (!p)
        return false;

    s_FilePath = filePath;
    strncpy(s_WorkingDirectory, filePath, p - filePath);

    std::string cmd = "hg log --template \"{rev}|{author|user}|{node|short}|" \
        "{date | shortdate}|{count(file_mods)}|{desc | strip | firstline}\\n\" " \
        "--no-merges -l 50 \"" + std::string(filePath) + "\"";
    std::string output = execute_command(cmd.c_str(), s_WorkingDirectory);

    auto changes = split(output, '\n');
    for (const auto& c: changes)
    {
        auto infos = split(c, '|');
        Revision r;
        r.id = std::stoi(infos[0]);
        r.author = infos[1];
        r.rev = infos[2];
        r.date = infos[3];
        r.mods = std::stoi(infos[4]);
        r.description = infos[5];

        s_Revisions.push_back(r);
    }

    std::sort(s_Revisions.begin(), s_Revisions.end(), [](const Revision& a, const Revision& b)
    {
        return a.id < b.id;
    });

    return !output.empty();
}

std::string execute_command(const char* cmd, const char* working_directory)
{
    std::string strResult;
    HANDLE hPipeRead, hPipeWrite;

    SECURITY_ATTRIBUTES saAttr = { sizeof(SECURITY_ATTRIBUTES) };
    saAttr.bInheritHandle = TRUE;   //Pipe handles are inherited by child process.
    saAttr.lpSecurityDescriptor = nullptr;

    // Create a pipe to get results from child's stdout.
    if (!CreatePipe(&hPipeRead, &hPipeWrite, &saAttr, 0))
        return strResult;

    STARTUPINFOA si = { sizeof(STARTUPINFOA) };
    si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
    si.hStdOutput = hPipeWrite;
    si.hStdError = hPipeWrite;
    si.wShowWindow = SW_HIDE;       // Prevents cmd window from flashing. Requires STARTF_USESHOWWINDOW in dwFlags.

    PROCESS_INFORMATION pi = { nullptr };

    const BOOL fSuccess = CreateProcessA(nullptr, (LPSTR)cmd, nullptr, nullptr, TRUE, CREATE_NEW_CONSOLE, nullptr, 
        working_directory ? working_directory : s_WorkingDirectory, &si, &pi);
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

            if (!::ReadFile(hPipeRead, buf, min(sizeof(buf) - 1, dwAvail), &dwRead, nullptr) || !dwRead)
                // error, the child process might ended
                break;

            buf[dwRead] = 0;
            strResult += buf;
        }
    } //for

    CloseHandle(hPipeWrite);
    CloseHandle(hPipeRead);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return strResult;
}

void render(int windowWidth, int windowHeight)
{
    const char* k_MainWindowTitle = "Timelapse";

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;

    ImGui::Begin(k_MainWindowTitle, nullptr,
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse |
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_MenuBar);
    ImGui::SetWindowPos(k_MainWindowTitle, ImVec2(0, 0));
    ImGui::SetWindowSize(k_MainWindowTitle, ImVec2((float)windowWidth, (float)windowHeight));

    static int revision = (int)s_Revisions.size() - 1;
    
    ImGui::PushItemWidth(-1);
    ImGui::Text("Revision: %s - %s", s_FilePath.c_str(), revision >= 0 ? s_Revisions[revision].rev.c_str() : "");

    if (!s_Revisions.empty())
    {
        Revision& r = s_Revisions[revision];
        ImGui::SliderInt("", &revision, 0, (int)s_Revisions.size()-1);

        if (r.source.empty())
        {
            std::string cmd = "hg annotate --user -c -w -b -B -r " + r.rev + " \"" + s_FilePath + "\"";
            r.source = execute_command(cmd.c_str(), s_WorkingDirectory);
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
    ImGui::End();
}

}
