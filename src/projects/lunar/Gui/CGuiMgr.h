#pragma once

#include "CGuiPanel.h"

#include <vector>
#include <d3d9.h>

class CGuiMgr
{
public:
    CGuiMgr();
    ~CGuiMgr();

public:
    bool Initialize(HWND hWnd, IDirect3DDevice9* pDevice);
    void Shutdown();
    bool MessageHandler(HWND, UINT, WPARAM, LPARAM);

    void RegisterPanels();
    void AddPanel(CGuiPanel* panel);
    void RemovePanel(CGuiPanel* panel);
    void Render();

    HWND GetWindow() { return m_hWnd; }
    IDirect3DDevice9* GetDevice() { return m_pDevice; }

private:
    std::vector<CGuiPanel*> m_Panels;

private:
    HWND m_hWnd;
    IDirect3DDevice9* m_pDevice;

private:
    void BeginFrame();
    void EndFrame();
};

namespace Global { inline CGuiMgr LunarGui; }