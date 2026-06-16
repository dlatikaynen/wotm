#pragma once

#include "main.h"
#include "const.h"

// level background is built from a stream of drawing commands from lua
void level_begin(SDL_Renderer* r, Gamestate* state, int w, int h);
float level_advance(Gamestate* state); // runs one frame's command(s) returning 0..1
void level_complete(Gamestate* state);
void level_step(void* userData);
