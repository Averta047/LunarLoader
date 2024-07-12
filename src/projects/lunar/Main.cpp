#include "CLuaManager.h"

#include <iostream>

int main()
{
	Global::LuaManager.Initialize();

	Global::LuaManager.LoadScript("script1.lua");
	Global::LuaManager.LoadScript("script2.lua");
	Global::LuaManager.LoadScript("script3.lua");

	while (true)
	{
		for (int i = 0; i < Global::LuaManager.m_Scripts.size(); i++)
		{
			std::cout << "Currently Running Scripts : " << Global::LuaManager.m_Scripts[i].m_pName << std::endl;
		}

		Global::LuaManager.Update();
	}

	return EXIT_SUCCESS;
}