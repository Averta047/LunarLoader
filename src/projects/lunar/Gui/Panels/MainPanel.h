#pragma once

#include "../CGuiPanel.h"
//#include "../CGuiWidgets.h"

class CCodeEditor;
class MainPanel : public CGuiPanel/*, public CGuiWidgets*/ {
public:
    MainPanel();
    ~MainPanel();

    void Render() override;

private:
    CCodeEditor* m_pCodeEditor;
};