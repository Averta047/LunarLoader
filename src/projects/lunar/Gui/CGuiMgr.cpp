#include "CGuiMgr.h"

// Gui
#include "imgui.h"
#include "imgui_internal.h"

// Gui Backends
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

// Gui Addons
//#include "imgui_freetype.h"
#include "imgui_addons.h"

// Fonts
#include "../../Resources/Fonts/MuseoSans300.h"

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
    style.WindowRounding    = 5;
    style.ChildRounding     = 6;
    style.FrameRounding     = 6;
    style.GrabRounding      = 3;
    style.PopupRounding     = 6;
    style.TabRounding       = 8;
    style.ScrollbarRounding = 3;

    style.ButtonTextAlign   = { 0.5f, 0.5f };
    style.WindowTitleAlign  = { 0.5f, 0.5f };
    style.FramePadding      = { 9.0f, 9.0f };
    style.WindowPadding     = { 20.0f, 20.0f };
    style.ItemSpacing       = { 8.0f, 8.0f };
    style.ItemInnerSpacing  = { style.WindowPadding.x, style.FramePadding.y };

    style.WindowBorderSize  = 1;
    style.FrameBorderSize   = 1;

    style.ScrollbarSize     = 12.f;
    style.GrabMinSize       = 8.f;
    
    style.Colors[ImGuiCol_WindowBg]             = ImAdd::HexToColorVec4(0x262626, 1.0f);
    style.Colors[ImGuiCol_ChildBg]              = ImAdd::HexToColorVec4(0x262626, 1.0f);
    style.Colors[ImGuiCol_PopupBg]              = ImAdd::HexToColorVec4(0x262626, 1.0f);
    style.Colors[ImGuiCol_MenuBarBg]            = ImAdd::HexToColorVec4(0x262626, 1.0f);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImAdd::HexToColorVec4(0x262626, 1.0f);

    style.Colors[ImGuiCol_Border]               = ImAdd::HexToColorVec4(0x404040, 1.0f);
    style.Colors[ImGuiCol_Separator]            = style.Colors[ImGuiCol_Border];

    style.Colors[ImGuiCol_CheckMark]            = ImAdd::HexToColorVec4(0xFFFFFF, 1.0f);
    style.Colors[ImGuiCol_Text]                 = ImAdd::HexToColorVec4(0xFFFFFF, 1.0f);
    style.Colors[ImGuiCol_TextDisabled]         = ImAdd::HexToColorVec4(0x949494, 1.0f);

    style.Colors[ImGuiCol_SliderGrab]           = ImAdd::HexToColorVec4(0xFF003D, 1.0f);
    style.Colors[ImGuiCol_SliderGrabActive]     = ImAdd::HexToColorVec4(0xA80F34, 1.0f);

    style.Colors[ImGuiCol_Button]               = ImAdd::HexToColorVec4(0x2F2F2F, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered]        = ImAdd::HexToColorVec4(0x2C2C2C, 1.0f);
    style.Colors[ImGuiCol_ButtonActive]         = ImAdd::HexToColorVec4(0x2A2A2A, 1.0f);

    style.Colors[ImGuiCol_FrameBg]              = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_FrameBgHovered]       = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_FrameBgActive]        = style.Colors[ImGuiCol_ButtonActive];

    style.Colors[ImGuiCol_Header]               = style.Colors[ImGuiCol_Button];
    style.Colors[ImGuiCol_HeaderHovered]        = style.Colors[ImGuiCol_ButtonHovered];
    style.Colors[ImGuiCol_HeaderActive]         = style.Colors[ImGuiCol_ButtonActive];

    //ImFontConfig cfg;
    //cfg.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_ForceAutoHint;
    io.Fonts->AddFontFromMemoryCompressedTTF(MuseoSans300_compressed_data, MuseoSans300_compressed_size, 15, nullptr, io.Fonts->GetGlyphRangesDefault());

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