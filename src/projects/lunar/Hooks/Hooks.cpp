#include "Hooks.h"
#include "../../CConsole.h"

#include "../Gui/CGuiMgr.h"

LRESULT __stdcall HookedWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Global::LunarGui.MessageHandler(hWnd, uMsg, wParam, lParam);

	return CallWindowProcA(Global::Hooks.GetOriginalWndProc(), hWnd, uMsg, wParam, lParam);
}

void CGlobal_Hooks::Initialize()
{
	XASSERT(MH_Initialize() != MH_STATUS::MH_OK);
	Global::Console.Print("MinHook: initialized");

	Hooks::Direct3DDevice9_EndScene::Initialize();
	Global::Console.Print("MinHook: Direct3DDevice9::EndScene() hooked");

	HWND hWnd = FindWindowA("Valve001", nullptr);
	//HWND hWnd = Global::LunarGui.GetWindow();
	this->HookWndProc(hWnd);
	Global::Console.Print("MinHook: WndProc() hooked");
	
	XASSERT(MH_EnableHook(MH_ALL_HOOKS) != MH_STATUS::MH_OK);
	Global::Console.Print("MinHook: All hooks enabled");
}

void CGlobal_Hooks::Uninitialize()
{
	//Global::LunarGui.Shutdown(); // Crach <3
	HWND hWnd = FindWindowA("Valve001", nullptr);
	UnhookWndProc(hWnd);
	XASSERT(MH_Uninitialize() != MH_STATUS::MH_OK);
}

void CGlobal_Hooks::HookWndProc(HWND hWnd)
{
	if (hWnd) {
		OriginalWndProc = reinterpret_cast<WNDPROC>(SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(HookedWndProc)));
	}
}

void CGlobal_Hooks::UnhookWndProc(HWND hWnd)
{
	SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(OriginalWndProc));
}