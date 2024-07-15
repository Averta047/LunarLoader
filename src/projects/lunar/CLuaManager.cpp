#include "CLuaManager.h"
#include "Lua.hpp"

#include <iostream>

void CLuaManager::Initialize()
{

}

void CLuaManager::Uninitialize()
{
	if (m_Scripts.empty())
		return;

	for (int i = 0; i < m_Scripts.size(); i++)
	{
		UnloadScript(m_Scripts[i].m_pLuaState);
//		delete& m_Scripts[i];
	}
}

void CLuaManager::LoadScript(const char* name)
{
	lua_Script luaScript = { 0 };
	luaScript.m_pName = name;
	luaScript.m_pLuaState = luaL_newstate();

	luaL_openlibs(luaScript.m_pLuaState);

	if (luaL_dofile(luaScript.m_pLuaState, luaScript.m_pName) != LUA_OK) 
	{
		const char* error = lua_tostring(luaScript.m_pLuaState, -1);
		std::cerr << "Error loading script '" << luaScript.m_pName << "': " << error << std::endl;
		lua_pop(luaScript.m_pLuaState, 1);
		lua_close(luaScript.m_pLuaState);
	}
	else 
	{
		m_Scripts.push_back(luaScript);
	}
}

void CLuaManager::UnloadScript(lua_State* pLuaState)
{
	auto it = std::find_if(m_Scripts.begin(), m_Scripts.end(),
		[pLuaState](const lua_Script& script) {
			return script.m_pLuaState == pLuaState;
		});

	if (it != m_Scripts.end()) {
		lua_close(it->m_pLuaState);
		m_Scripts.erase(it);
	}
}

void CLuaManager::Update()
{
	auto it = m_Scripts.begin();
	while (it != m_Scripts.end())
	{
		if (lua_pcall(it->m_pLuaState, 0, 0, 0) != LUA_OK)
		{
			//const char* error = lua_tostring(it->m_pLuaState, -1);
			//std::cerr << "Error executing script '" << it->m_pName << "': " << error << std::endl;
			std::cerr << it->m_pName << ": ended" << std::endl;
			lua_pop(it->m_pLuaState, 1);
			lua_close(it->m_pLuaState);
			it = m_Scripts.erase(it);
		}
		else
		{
			++it;
		}
	}
}
