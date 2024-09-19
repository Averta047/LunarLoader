#include "CGuiMgr.h"

// Panels
#include "Panels/MainPanel.h"

void CGuiMgr::RegisterPanels()
{
	MainPanel* pMainPanel = new MainPanel();
	this->AddPanel(pMainPanel);
}