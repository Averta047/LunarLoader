#include "CLuaManager.h"
#include "CConsole.h"

#include "Hooks/Hooks.h"

#include <iostream>
#include <windows.h>

void MainThread(HMODULE hInstance)
{
	Global::Console.Init("LunarLoader - Console - Build: " __DATE__, false, true);
	Global::Console.Print("LunarLoader Injected !\nInitializing core...");

	Global::Hooks.Initialize();
	
	if (Global::LuaManager.Initialize()) {
		Global::Console.Print("CLuaManager initialized");
	}
	else {
		Global::Console.Print("Faild initializing CLuaManager. exiting...");
		FreeLibraryAndExitThread(hInstance, EXIT_FAILURE);
	}

	Global::LuaManager.LoadScript("core.lua");

	while (!GetAsyncKeyState(VK_END))
	{
		for (size_t i = 0; i < Global::LuaManager.m_Scripts.size(); i++)
		{
			std::cout << "Currently Running Scripts : " << Global::LuaManager.m_Scripts[i].m_pName << std::endl;
		}

		Global::LuaManager.Update();
		
		Sleep(10);
	}

	Global::LuaManager.Uninitialize();
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