#include "Hooks.h"
#include "../../CConsole.h"

void CGlobal_Hooks::Initialize()
{
	XASSERT(MH_Initialize() != MH_STATUS::MH_OK);
	Global::Console.Print("MinHook: initialized");

	Hooks::Direct3DDevice9_EndScene::Initialize();
	Global::Console.Print("MinHook: Direct3DDevice9::EndScene(...) hooked");
	
	Hooks::Direct3DDevice9_Present::Initialize();
	Global::Console.Print("MinHook: Direct3DDevice9::Present(...) hooked");
	
	Hooks::Direct3DDevice9_Reset::Initialize();
	Global::Console.Print("MinHook: Direct3DDevice9::Reset(...) hooked");

	XASSERT(MH_EnableHook(MH_ALL_HOOKS) != MH_STATUS::MH_OK);
	Global::Console.Print("MinHook: all hooks enabled");
}

void CGlobal_Hooks::Uninitialize()
{
	XASSERT(MH_Uninitialize() != MH_STATUS::MH_OK);
}
