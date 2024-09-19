#include "../Hooks.h"

#include "../../Gui/CGuiMgr.h"

#include <d3d9.h>
#include <d3dx9.h>

DEFINE_HOOK(Direct3DDevice9_EndScene, void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice)
{
	Func.Original<FN>()(ecx, edx, pDevice);

	static D3DDEVICE_CREATION_PARAMETERS pParameters;
	static bool init = true;

	if (init) 
	{
		pDevice->GetCreationParameters(&pParameters);

		Global::LunarGui.Initialize(pParameters.hFocusWindow, pDevice);

		init = false;
	}

	pDevice->SetRenderState(D3DRS_SRGBWRITEENABLE, FALSE);

	Global::LunarGui.Render();
}