#include "CGuiMgr.h"

// Gui
#include "imgui.h"
#include "imgui_internal.h"

// Gui Backends
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

// Gui Addons
//#include "imgui_freetype.h"

CGuiMgr::CGuiMgr()
{
    m_hWnd = 0;
    m_pDevice = 0;
}

CGuiMgr::~CGuiMgr()
{
    if (!m_Panels.empty())
    {
        for (CGuiPanel* panel : m_Panels)
        {
            delete panel;
        }

        m_Panels.clear();
    }
}

bool CGuiMgr::Initialize(HWND hWnd, IDirect3DDevice9* pDevice)
{
    bool result = true;

    m_hWnd = hWnd;
    m_pDevice = pDevice;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiStyle& style = ImGui::GetStyle();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    io.IniFilename = nullptr;               // Disable INI File  
    GImGui->NavDisableHighlight = true;     // Disable Highlighting

    if (!GImGui->NavDisableHighlight) {
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;   // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;    // Enable Gamepad Controls
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Custom styles
    style.WindowRounding        = 4;
    style.ChildRounding         = 3;
    style.FrameRounding         = 2;
    style.GrabRounding          = 1;
    style.PopupRounding         = 2;
    style.TabRounding           = 2;
    style.ScrollbarRounding     = 1;

    style.ButtonTextAlign       = { 0.5f, 0.5f };
    style.WindowTitleAlign      = { 0.5f, 0.5f };
    style.FramePadding          = { 8.0f, 8.0f };
    style.WindowPadding         = { 10.0f, 10.0f };
    style.ItemSpacing           = style.WindowPadding;
    style.ItemInnerSpacing      = { style.FramePadding.x, 2 };

    style.WindowBorderSize      = 1;
    style.FrameBorderSize       = 1;
    style.PopupBorderSize       = 1;

    style.ScrollbarSize = 12.f;
    style.GrabMinSize = 8.f;

    /*
    ImFontConfig cfg;
    cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.Fonts->AddFontFromMemoryCompressedTTF(Poppins_Medium_compressed_data, Poppins_Medium_compressed_size, 16, &cfg, io.Fonts->GetGlyphRangesDefault());
    */

    // Setup Platform/Renderer backends
    result &= ImGui_ImplWin32_Init(hWnd);
    result &= ImGui_ImplDX9_Init(pDevice);

    this->RegisterPanels();

    return result;
}

void CGuiMgr::Shutdown()
{
    ImGui_ImplDX9_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void CGuiMgr::AddPanel(CGuiPanel* panel)
{
    m_Panels.push_back(panel);
}

void CGuiMgr::RemovePanel(CGuiPanel* panel)
{
    if (!m_Panels.empty())
    {
        auto it = std::find(m_Panels.begin(), m_Panels.end(), panel);
        if (it != m_Panels.end())
        {
            m_Panels.erase(it);
            delete panel;
        }
    }
}

void CGuiMgr::Render()
{
    BeginFrame();
    if (!m_Panels.empty())
    {
        for (CGuiPanel* panel : m_Panels)
        {
            panel->Render();
        }
    }
    EndFrame();
}

void CGuiMgr::BeginFrame()
{
    ImGui_ImplDX9_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

void CGuiMgr::EndFrame()
{
    ImGui::Render();
    ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
bool CGuiMgr::MessageHandler(HWND hwnd, UINT umsg, WPARAM wparam, LPARAM lparam)
{
    return ImGui_ImplWin32_WndProcHandler(hwnd, umsg, wparam, lparam);
}