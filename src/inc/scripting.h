#pragma once

// Step 1 integration probe: runs a tiny Lua script through sol2 to prove the
// scripting layer compiles and links into the wasm build. Binds a C++ function,
// calls it from Lua, and returns the value the script computes (42 on success),
// or -1 if the script errored. Logged from main() at startup.
int scripting_smoke_test();
