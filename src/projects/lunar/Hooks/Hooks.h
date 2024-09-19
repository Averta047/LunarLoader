#pragma once

#include "Hook.h"
#include "../Utils/VFunc.h"

#include <d3d9.h>

namespace Interfaces { inline IDirect3DDevice9* Direct3DDevice9 = nullptr; }

CREATE_HOOK(Direct3DDevice9_EndScene, Util::VFunc.Get<void*>(Interfaces::Direct3DDevice9, 42u), void, __fastcall, void* ecx, void* edx, IDirect3DDevice9* pDevice);

class CGlobal_Hooks
{
public:
	void Initialize();
	void Uninitialize();

	void HookWndProc(HWND hWnd);
	void UnhookWndProc(HWND hWnd);

	WNDPROC GetOriginalWndProc() { return OriginalWndProc; }
private:
	WNDPROC OriginalWndProc;
};

namespace Global { inline CGlobal_Hooks Hooks; }