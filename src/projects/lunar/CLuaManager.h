#pragma once

#include <vector>
#include <sol/sol.hpp>

struct lua_Script
{
public:
    sol::state m_LuaState;
    const char* m_pName;
};

class CLuaManager
{
public:
    bool Initialize();
    void Uninitialize();

    bool LoadScript(const char* name);
    void UnloadScript(const char* name);

    void Update();

public:
    std::vector<lua_Script> m_Scripts;
};

namespace Global { inline CLuaManager LuaManager; }
 