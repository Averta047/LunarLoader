#include "CLuaManager.h"
#include "CConsole.h"
#include <iostream>

bool CLuaManager::Initialize()
{
    return true;
}

void CLuaManager::Uninitialize()
{
    if (m_Scripts.empty())
        return;

    for (size_t i = 0; i < m_Scripts.size(); i++)
    {
        UnloadScript(m_Scripts[i].m_pName);
    }
}

bool CLuaManager::LoadScript(const char* name)
{
    lua_Script luaScript;
    luaScript.m_pName = name;
    luaScript.m_LuaState.open_libraries(sol::lib::base, sol::lib::package, sol::lib::coroutine,
        sol::lib::string, sol::lib::os, sol::lib::math,
        sol::lib::table, sol::lib::debug, sol::lib::bit32,
        sol::lib::io, sol::lib::ffi, sol::lib::jit);

    sol::load_result script = luaScript.m_LuaState.load_file(name);
    if (!script.valid())
    {
        sol::error err = script;
        Global::Console.Print("Error loading script '%s': %s", name, err.what());
        return false;
    }
    else
    {
        sol::protected_function_result result = script();
        if (!result.valid())
        {
            sol::error err = result;
            Global::Console.Print("Error running script '%s': %s", name, err.what());
            return false;
        }

        m_Scripts.push_back(std::move(luaScript));
        return true;
    }
}

void CLuaManager::UnloadScript(const char* name)
{
    auto it = std::find_if(m_Scripts.begin(), m_Scripts.end(),
        [name](const lua_Script& script) {
            return strcmp(script.m_pName, name) == 0;
        });

    if (it != m_Scripts.end())
    {
        m_Scripts.erase(it);
    }
}

void CLuaManager::Update()
{
    auto it = m_Scripts.begin();
    while (it != m_Scripts.end())
    {
        sol::protected_function_result result = it->m_LuaState.safe_script("return 0", sol::script_pass_on_error);
        if (!result.valid())
        {
            sol::error err = result;
            Global::Console.Print("%s: ended with error %s", it->m_pName, err.what());
            it = m_Scripts.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
