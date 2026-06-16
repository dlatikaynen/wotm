#include "inc/scripting.h"

#include <sol/sol.hpp>
#include <iostream>

int LUA_Init()
{
    sol::state lua;
    lua.open_libraries(sol::lib::base, sol::lib::math);
    lua.set_function("answer", [](int a, int b) { return a * b; });

    auto result = lua.safe_script("return answer(6, 7)", sol::script_pass_on_error);

    if (!result.valid())
    {
        sol::error err = result;
        std::cerr << "Lua binding failed: " << err.what() << "\n";

        return -1;
    }

    const int value = result;

    std::cout << "Lua script host version " << LUA_VERSION << std::endl;

    return value;
}
