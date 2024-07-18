#include "../Hooks.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

DEFINE_HOOK(Direct3DDevice9_EndScene, void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice)
{
	Func.Original<FN>()(ecx, edx, pDevice);

	static D3DDEVICE_CREATION_PARAMETERS pParameters;
	static bool init = true;

	if (init) 
	{
		pDevice->GetCreationParameters(&pParameters);

		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiStyle& style = ImGui::GetStyle();
		ImGuiIO& io = ImGui::GetIO(); (void)io;

		io.IniFilename = nullptr;               // Disable INI File  
		GImGui->NavDisableHighlight = true;     // Disable Highlighting

		ImGui::StyleColorsDark();

		ImGui_ImplWin32_Init(pParameters.hFocusWindow);
		ImGui_ImplDX9_Init(pDevice);

		//Global::Menu.Initialize(FindWindow("Valve001", nullptr), pDevice);
		init = false;
	}

	//pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);
	//Global::Menu.Render();
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	{
		//Global::LuaMgr.RenderThink();
		ImGui::Begin("Test Window");
		{

		}
		ImGui::End();
	}
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

	// bad idea
	//ImGui_ImplDX9_InvalidateDeviceObjects();
}