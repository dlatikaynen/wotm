#include "inc/scripting.h"

#include <sol/sol.hpp>
#include <iostream>

int scripting_smoke_test()
{
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math);

    // a host function exposed to Lua, to exercise the C++ <-> Lua boundary
    lua.set_function("answer", [](int a, int b) { return a * b; });

    auto result = lua.safe_script("return answer(6, 7)", sol::script_pass_on_error);

    if (!result.valid())
    {
        sol::error err = result;
        std::cout << "[lua] smoke test FAILED: " << err.what() << std::endl;
        return -1;
    }

    const int value = result;
    std::cout << "[lua] " << LUA_VERSION << " + sol2 ok, script returned " << value << std::endl;
    return value;
}
