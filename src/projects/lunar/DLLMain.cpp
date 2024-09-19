#include "CLuaManager.h"
#include "CConsole.h"

#include "Utils/Pattern.h"
#include "Hooks/Hooks.h"

#include <iostream>
#include <windows.h>

void MainThread(HMODULE hInstance)
{
	Global::Console.Init("LunarLoader - Console - Build: " __DATE__, false, true);
	Global::Console.Print("LunarLoader Injected !\nInitializing core...");

	Interfaces::Direct3DDevice9 = **(IDirect3DDevice9***)(Util::Pattern.Find("shaderapidx9.dll", "55 8B EC 51 56 8B F1 83 7E 24 00") + 0x2B);

	Global::Hooks.Initialize();

	while (!GetAsyncKeyState(VK_F11))
		Sleep(420);
	
	Global::Hooks.Uninitialize();
	Global::Console.Close();

	FreeLibraryAndExitThread(hInstance, EXIT_SUCCESS);
}

BOOL WINAPI DllMain( HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpReserved )
{
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		const HANDLE thread = CreateThread(
			nullptr,
			0,
			reinterpret_cast<LPTHREAD_START_ROUTINE>(MainThread),
			hinstDLL,
			0,
			nullptr
		);

		if (thread)
			CloseHandle(thread);
	}

	return TRUE;
}