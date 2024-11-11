#include "MainPanel.h"
#include "../CCodeEditor.h"

#include "imgui.h"

#include <string>

MainPanel::MainPanel()
{
    m_pCodeEditor = new CCodeEditor();
    m_pCodeEditor->SetLanguageDefinition(CCodeEditor::LanguageDefinition::Lua());
}

MainPanel::~MainPanel()
{
    if (m_pCodeEditor != nullptr)
    {
        delete m_pCodeEditor;
    }
}

void MainPanel::Render() 
{
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::SetNextWindowSize(ImVec2(730, 500), ImGuiCond_Once);
    ImGui::Begin("MainPanel", nullptr, ImGuiWindowFlags_NoDecoration);
    {
        ImGui::BeginChild("Header", ImVec2(0, ImGui::GetFrameHeight()), ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground);
        {

        }
        ImGui::EndChild();
        m_pCodeEditor->Render("CodeEditor", ImVec2(200, 100), false);
    }
    ImGui::End();
}