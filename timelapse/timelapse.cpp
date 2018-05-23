#include "stdafx.h"
#include "timelapse.h"

#include <imgui/imgui.h>

namespace tl {

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
    
    ImGui::PushItemWidth(-1);
    static int revision = 0;
    ImGui::Text("Revision");
    ImGui::SliderInt("", &revision, -101, 100);

    ImGui::BeginChild("CodeView", ImVec2(ImGui::GetWindowContentRegionWidth(), 0), true, ImGuiWindowFlags_HorizontalScrollbar);
    for (int i = 0; i < 100; i++)
    {
        ImGui::Text("%04d: scrollable region", i);
        //if (goto_line && line == i)
          //  ImGui::SetScrollHere();
    }
    //if (goto_line && line >= 100)
      //  ImGui::SetScrollHere();
    ImGui::EndChild();
    
    ImGui::PopItemWidth();
    ImGui::End();
}

}
