#pragma once

#include "Hook.h"
#include "../Utils/VFunc.h"

// PluginSDK
#include "plugin.h"

// IDirect3DDevice9
#include <d3d9.h>
#include <d3dx9.h>

CREATE_HOOK(Direct3DDevice9_EndScene, Util::VFunc.Get<void*>((IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice(), 42u), void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice);
CREATE_HOOK(Direct3DDevice9_Present, Util::VFunc.Get<void*>((IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice(), 17u), void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice, CONST RECT* pSrcRect, CONST RECT* pDestRect, HWND hDestWindow, CONST RGNDATA* pDirtyRegion);
CREATE_HOOK(Direct3DDevice9_Reset, Util::VFunc.Get<void*>((IDirect3DDevice9*)RwD3D9GetCurrentD3DDevice(), 16u), void, __fastcall, void* ecx, void* edx, LPDIRECT3DDEVICE9 pDevice, D3DPRESENT_PARAMETERS* pPresentParams);

class CGlobal_Hooks
{
public:
	void Initialize();
	void Uninitialize();
};

namespace Global { inline CGlobal_Hooks Hooks; }