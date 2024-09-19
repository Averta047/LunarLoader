#include "MainPanel.h"

#include "imgui.h"
#include <string>

MainPanel::MainPanel()
{
}

MainPanel::~MainPanel()
{
}

void MainPanel::Render() {
    ImGui::Begin("Main Panel");
    {
        ImGui::Text("empty");
    }
    ImGui::End();
}