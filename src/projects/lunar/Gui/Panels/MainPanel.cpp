#include "MainPanel.h"

#include "imgui.h"
#include "imgui_lua_editor.h"

#include <string>

TextEditor g_LuaInput;

MainPanel::MainPanel()
{

}

MainPanel::~MainPanel()
{
}

void MainPanel::Render() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::SetNextWindowSize(ImVec2(520, 330), ImGuiCond_Once);
    ImGui::Begin("LunarLoader - Averta047");
    {
        g_LuaInput.Render("Editor", ImVec2(ImGui::GetWindowWidth() - style.WindowPadding.x * 2, ImGui::GetWindowHeight() - ImGui::GetFrameHeight() * 2 - style.WindowPadding.y * 2 - style.ItemSpacing.y), false);
        
        ImGui::Spacing();
        ImGui::BeginGroup();
        {
            ImGui::Button("Clear");

            ImGui::SameLine(ImGui::GetWindowWidth() / 2 - (ImGui::CalcTextSize("Open FileSave File").x + style.FramePadding.x * 4 + style.ItemSpacing.x) / 2);
            ImGui::Button("Open File");
            ImGui::SameLine();
            ImGui::Button("Save Script");

            ImGui::SameLine(ImGui::GetWindowWidth() - ImGui::CalcTextSize("Execute").x - style.FramePadding.x * 2 - style.WindowPadding.x - style.ItemSpacing.x);
            ImGui::Button("Execute");
        }
        ImGui::EndGroup();
    }
    ImGui::End();
}