#include "../Hooks.h"

#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_dx9.h"
#include "imgui_impl_win32.h"

DEFINE_HOOK(Direct3DDevice9_Reset, void, __fastcall, void* ecx, void* edx, LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentParams)
{
	Func.Original<FN>()(ecx, edx, pDevice, pPresentParams);

	//ImGui_ImplDX9_InvalidateDeviceObjects();
}