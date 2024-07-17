#pragma once

#include <vector>

struct lua_State;
struct lua_Script
{
public:
	lua_State* m_pLuaState;
	const char* m_pName;
};

class CLuaManager
{
public:
	bool Initialize();
	void Uninitialize();

	bool LoadScript(const char* name);
	void UnloadScript(lua_State* pLuaState);

	void Update();

public: // private:
	std::vector<lua_Script> m_Scripts;
};

namespace Global { inline CLuaManager LuaManager; }