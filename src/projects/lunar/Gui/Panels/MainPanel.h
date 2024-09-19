#pragma once

#include "../CGuiPanel.h"
//#include "../CGuiWidgets.h"

class MainPanel : public CGuiPanel/*, public CGuiWidgets*/ {
public:
    MainPanel();
    ~MainPanel();

    void Render() override;
};